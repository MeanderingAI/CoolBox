#include "../__rej_proxy_service/include/proxy_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <algorithm>

namespace services {
namespace proxy {

// ProxyServer implementation
ProxyServer::ProxyServer(const ProxyConfig& config)
    : config_(config)
    , running_(false)
    , server_socket_(-1)
    , cache_(std::make_unique<data_structures::ConcurrentHashMap<std::string, std::shared_ptr<CachedResponse>>>())
    , requests_count_(0)
    , cache_hits_(0)
    , blocked_requests_(0) {
}

ProxyServer::~ProxyServer() {
    stop();
}

bool ProxyServer::start() {
    if (running_) {
        return false;
    }
    
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return false;
    }
    
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config_.port);
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_socket_);
        return false;
    }
    
    if (listen(server_socket_, 10) < 0) {
        close(server_socket_);
        return false;
    }
    
    running_ = true;
    server_thread_ = std::thread(&ProxyServer::run_server, this);
    
    return true;
}

void ProxyServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

bool ProxyServer::is_running() const {
    return running_;
}

void ProxyServer::set_config(const ProxyConfig& config) {
    config_ = config;
}

ProxyConfig ProxyServer::get_config() const {
    return config_;
}

void ProxyServer::block_domain(const std::string& domain) {
    config_.blocked_domains.push_back(domain);
}

void ProxyServer::unblock_domain(const std::string& domain) {
    auto& domains = config_.blocked_domains;
    domains.erase(std::remove(domains.begin(), domains.end(), domain), domains.end());
}

bool ProxyServer::is_blocked(const std::string& domain) const {
    const auto& domains = config_.blocked_domains;
    return std::find(domains.begin(), domains.end(), domain) != domains.end();
}

void ProxyServer::clear_cache() {
    cache_->clear();
}

size_t ProxyServer::get_cache_size() const {
    return cache_->size();
}

void ProxyServer::enable_caching(bool enable) {
    config_.enable_caching = enable;
}

uint64_t ProxyServer::get_requests_count() const {
    return requests_count_.load();
}

uint64_t ProxyServer::get_cache_hits() const {
    return cache_hits_.load();
}

uint64_t ProxyServer::get_blocked_requests() const {
    return blocked_requests_.load();
}

void ProxyServer::run_server() {
    while (running_) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_address, &client_len);
        
        if (client_socket < 0) {
            if (running_) {
                continue;
            }
            break;
        }
        
        std::thread(&ProxyServer::handle_client, this, client_socket).detach();
    }
}

void ProxyServer::handle_client(int client_socket) {
    requests_count_++;
    
    char buffer[8192];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    std::string request(buffer, bytes_read);
    std::string method, host, path;
    std::string full_url = parse_request(request, method, host, path);
    
    if (is_blocked(host)) {
        blocked_requests_++;
        std::string blocked_response = "HTTP/1.1 403 Forbidden\r\n\r\nDomain blocked by proxy";
        send(client_socket, blocked_response.c_str(), blocked_response.length(), 0);
        close(client_socket);
        return;
    }
    
    // Check cache if enabled
    if (config_.enable_caching) {
        auto cached = get_cached_response(full_url);
        if (cached) {
            cache_hits_++;
            std::string response = cached->headers + "\r\n" + cached->body;
            send(client_socket, response.c_str(), response.length(), 0);
            close(client_socket);
            return;
        }
    }
    
    // Forward request
    std::string response;
    if (forward_request(host, 80, request, response)) {
        send(client_socket, response.c_str(), response.length(), 0);
        
        // Cache the response if enabled
        if (config_.enable_caching) {
            cache_response(full_url, response, "");
        }
    }
    
    close(client_socket);
}

std::string ProxyServer::parse_request(const std::string& request, std::string& method, std::string& host, std::string& path) {
    // Simplified HTTP request parsing
    size_t method_end = request.find(' ');
    if (method_end != std::string::npos) {
        method = request.substr(0, method_end);
    }
    
    size_t host_start = request.find("Host: ");
    if (host_start != std::string::npos) {
        host_start += 6;
        size_t host_end = request.find('\r', host_start);
        host = request.substr(host_start, host_end - host_start);
    }
    
    return "http://" + host + path;
}

bool ProxyServer::forward_request(const std::string& host, int port, const std::string& request, std::string& response) {
    // Simplified forward - would implement full HTTP proxy logic
    return false;
}

std::optional<CachedResponse> ProxyServer::get_cached_response(const std::string& url) {
    std::shared_ptr<CachedResponse> cached;
    if (cache_->get(url, cached)) {
        if (!cached->is_expired(3600)) { // 1 hour default
            return *cached;
        }
        cache_->remove(url);
    }
    return std::nullopt;
}

void ProxyServer::cache_response(const std::string& url, const std::string& headers, const std::string& body) {
    auto cached = std::make_shared<CachedResponse>();
    cached->headers = headers;
    cached->body = body;
    cached->cached_at = std::chrono::steady_clock::now();
    cached->size_bytes = headers.size() + body.size();
    
    cache_->insert(url, cached);
}

// ReverseProxy implementation
ReverseProxy::ReverseProxy(int port)
    : port_(port)
    , running_(false)
    , strategy_(Strategy::ROUND_ROBIN)
    , current_backend_index_(0)
    , total_requests_(0) {
}

ReverseProxy::~ReverseProxy() {
    stop();
}

void ReverseProxy::add_backend(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(backends_mutex_);
    backends_.push_back(std::make_shared<Backend>(host, port));
}

void ReverseProxy::remove_backend(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(backends_mutex_);
    backends_.erase(
        std::remove_if(backends_.begin(), backends_.end(),
            [&host, port](const std::shared_ptr<Backend>& b) {
                return b->host == host && b->port == port;
            }),
        backends_.end()
    );
}

std::vector<std::pair<std::string, int>> ReverseProxy::get_backends() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(backends_mutex_));
    std::vector<std::pair<std::string, int>> result;
    for (const auto& backend : backends_) {
        result.emplace_back(backend->host, backend->port);
    }
    return result;
}

void ReverseProxy::set_strategy(Strategy strategy) {
    strategy_ = strategy;
}

ReverseProxy::Strategy ReverseProxy::get_strategy() const {
    return strategy_;
}

bool ReverseProxy::start() {
    if (running_ || backends_.empty()) {
        return false;
    }
    
    running_ = true;
    return true;
}

void ReverseProxy::stop() {
    running_ = false;
}

bool ReverseProxy::is_running() const {
    return running_;
}

void ReverseProxy::enable_health_checks(bool enable, int interval_seconds) {
    // Would implement health check thread
}

bool ReverseProxy::is_backend_healthy(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(backends_mutex_);
    for (const auto& backend : backends_) {
        if (backend->host == host && backend->port == port) {
            return backend->healthy.load();
        }
    }
    return false;
}

uint64_t ReverseProxy::get_total_requests() const {
    return total_requests_.load();
}

uint64_t ReverseProxy::get_backend_requests(const std::string& host, int port) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(backends_mutex_));
    for (const auto& backend : backends_) {
        if (backend->host == host && backend->port == port) {
            return backend->request_count.load();
        }
    }
    return 0;
}

void ReverseProxy::run_server() {
    // Would implement reverse proxy server loop
}

void ReverseProxy::handle_client(int client_socket) {
    total_requests_++;
}

std::shared_ptr<ReverseProxy::Backend> ReverseProxy::select_backend(const std::string& client_ip) {
    std::lock_guard<std::mutex> lock(backends_mutex_);
    
    if (backends_.empty()) {
        return nullptr;
    }
    
    switch (strategy_) {
        case Strategy::ROUND_ROBIN: {
            size_t index = current_backend_index_++ % backends_.size();
            return backends_[index];
        }
        case Strategy::LEAST_CONNECTIONS: {
            return *std::min_element(backends_.begin(), backends_.end(),
                [](const std::shared_ptr<Backend>& a, const std::shared_ptr<Backend>& b) {
                    return a->active_connections < b->active_connections;
                });
        }
        case Strategy::RANDOM: {
            size_t index = rand() % backends_.size();
            return backends_[index];
        }
        default:
            return backends_[0];
    }
}

void ReverseProxy::run_health_checks() {
    // Would implement health check logic
}

bool ReverseProxy::check_backend_health(std::shared_ptr<Backend> backend) {
    return true; // Simplified
}

} // namespace proxy
} // namespace services
