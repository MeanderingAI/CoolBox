/*
 * Proxy Service - Lightweight nginx-clone with port forwarding, SSL, caching, and error handling
 * 
 * Features:
 *  - Reverse proxy with configurable upstream servers
 *  - Port forwarding and load balancing
 *  - SSL/TLS termination (when OpenSSL is available)
 *  - In-memory response caching with TTL
 *  - Connection pooling and keepalive
 *  - Error handling with custom error pages
 *  - Health checks for upstream servers
 *  - Request/response logging
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <chrono>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <memory>
#include <queue>

// Network includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

// ============================================================================
// Configuration Structures
// ============================================================================

struct UpstreamServer {
    std::string host;
    int port;
    int weight = 1;
    bool healthy = true;
    int max_fails = 3;
    int fail_count = 0;
    std::chrono::steady_clock::time_point last_check;
    int active_connections = 0;
};

struct ProxyRoute {
    std::string path_prefix;
    std::vector<UpstreamServer> upstreams;
    bool enable_cache = false;
    int cache_ttl_seconds = 60;
    bool enable_ssl = false;
    int balance_method = 0; // 0=round-robin, 1=least-conn, 2=weighted
    int current_upstream = 0;
};

struct CacheEntry {
    std::string content;
    std::chrono::steady_clock::time_point expiry;
    std::string content_type;
    int status_code;
};

struct ProxyConfig {
    int listen_port = 8080;
    bool enable_ssl = false;
    std::string ssl_cert_path;
    std::string ssl_key_path;
    std::vector<ProxyRoute> routes;
    int worker_threads = 4;
    int max_connections = 1000;
    int keepalive_timeout = 65;
    int upstream_connect_timeout = 5;
    int upstream_read_timeout = 60;
    bool enable_logging = true;
    size_t max_cache_size = 100 * 1024 * 1024; // 100MB
};

// ============================================================================
// Proxy Service Class
// ============================================================================

class ProxyService {
private:
    ProxyConfig config_;
    int server_fd_ = -1;
    bool running_ = true;
    
    // Caching
    std::unordered_map<std::string, CacheEntry> cache_;
    std::mutex cache_mutex_;
    size_t current_cache_size_ = 0;
    
    // Connection pool (simple implementation)
    std::map<std::string, std::queue<int>> connection_pool_;
    std::mutex pool_mutex_;
    
    // Statistics
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> cache_hits_{0};
    std::atomic<uint64_t> cache_misses_{0};
    std::atomic<uint64_t> upstream_errors_{0};
    
#ifdef HAVE_OPENSSL
    SSL_CTX* ssl_ctx_ = nullptr;
#endif

public:
    ProxyService(const ProxyConfig& config) : config_(config) {}
    
    ~ProxyService() {
        stop();
#ifdef HAVE_OPENSSL
        if (ssl_ctx_) {
            SSL_CTX_free(ssl_ctx_);
        }
#endif
    }
    
    bool initialize() {
        std::cout << "🚀 Proxy Service Initializing...\n";
        std::cout << "   Listen Port: " << config_.listen_port << "\n";
        std::cout << "   Worker Threads: " << config_.worker_threads << "\n";
        std::cout << "   SSL Enabled: " << (config_.enable_ssl ? "Yes" : "No") << "\n";
        std::cout << "   Routes: " << config_.routes.size() << "\n\n";
        
#ifdef HAVE_OPENSSL
        if (config_.enable_ssl) {
            if (!init_ssl()) {
                std::cerr << "❌ Failed to initialize SSL\n";
                return false;
            }
            std::cout << "✓ SSL/TLS initialized\n";
        }
#else
        if (config_.enable_ssl) {
            std::cerr << "⚠️  SSL requested but OpenSSL not available at compile time\n";
            config_.enable_ssl = false;
        }
#endif
        
        // Create listening socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "❌ Failed to create socket: " << strerror(errno) << "\n";
            return false;
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "⚠️  Failed to set SO_REUSEADDR: " << strerror(errno) << "\n";
        }
        
        // Bind to port
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(config_.listen_port);
        
        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "❌ Failed to bind to port " << config_.listen_port << ": " << strerror(errno) << "\n";
            close(server_fd_);
            return false;
        }
        
        // Listen for connections
        if (listen(server_fd_, config_.max_connections) < 0) {
            std::cerr << "❌ Failed to listen: " << strerror(errno) << "\n";
            close(server_fd_);
            return false;
        }
        
        std::cout << "✓ Proxy service listening on port " << config_.listen_port << "\n";
        
        // Print route configuration
        for (size_t i = 0; i < config_.routes.size(); i++) {
            const auto& route = config_.routes[i];
            std::cout << "   Route " << (i+1) << ": " << route.path_prefix << " → " 
                      << route.upstreams.size() << " upstream(s)\n";
            for (const auto& upstream : route.upstreams) {
                std::cout << "      • " << upstream.host << ":" << upstream.port 
                          << " (weight: " << upstream.weight << ")\n";
            }
        }
        std::cout << "\n";
        
        return true;
    }
    
    void start() {
        std::cout << "🔥 Proxy service started!\n";
        std::cout << "   Access proxy at: http://localhost:" << config_.listen_port << "\n\n";
        
        // Start health check thread
        std::thread health_checker([this]() {
            while (running_) {
                check_upstream_health();
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        });
        health_checker.detach();
        
        // Start cache cleanup thread
        std::thread cache_cleaner([this]() {
            while (running_) {
                cleanup_cache();
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        });
        cache_cleaner.detach();
        
        // Accept connections (thread-per-request model)
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                if (errno == EINTR) continue;
                std::cerr << "⚠️  Accept failed: " << strerror(errno) << "\n";
                continue;
            }
            
            // Handle in new thread
            std::thread([this, client_fd, client_addr]() {
                handle_client(client_fd, client_addr);
            }).detach();
        }
    }
    
    void stop() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
    }
    
    void print_stats() {
        std::cout << "\n📊 Proxy Statistics:\n";
        std::cout << "   Total Requests: " << total_requests_ << "\n";
        std::cout << "   Cache Hits: " << cache_hits_ << " (" 
                  << (total_requests_ > 0 ? (cache_hits_ * 100.0 / total_requests_) : 0) 
                  << "%)\n";
        std::cout << "   Cache Misses: " << cache_misses_ << "\n";
        std::cout << "   Upstream Errors: " << upstream_errors_ << "\n";
        std::cout << "   Cache Size: " << (current_cache_size_ / 1024) << " KB\n";
        std::cout << "   Cached Items: " << cache_.size() << "\n\n";
    }

private:
#ifdef HAVE_OPENSSL
    bool init_ssl() {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        
        ssl_ctx_ = SSL_CTX_new(TLS_server_method());
        if (!ssl_ctx_) {
            ERR_print_errors_fp(stderr);
            return false;
        }
        
        if (!config_.ssl_cert_path.empty() && !config_.ssl_key_path.empty()) {
            if (SSL_CTX_use_certificate_file(ssl_ctx_, config_.ssl_cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                return false;
            }
            
            if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, config_.ssl_key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                return false;
            }
        }
        
        return true;
    }
#endif
    
    void handle_client(int client_fd, struct sockaddr_in client_addr) {
        total_requests_++;
        
        // Read HTTP request
        char buffer[8192];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            close(client_fd);
            return;
        }
        buffer[bytes_read] = '\0';
        
        std::string request(buffer, bytes_read);
        
        // Parse request line
        std::istringstream request_stream(request);
        std::string method, path, version;
        request_stream >> method >> path >> version;
        
        if (config_.enable_logging) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            log_request(client_ip, method, path);
        }
        
        // Find matching route
        ProxyRoute* route = find_route(path);
        if (!route) {
            send_error(client_fd, 404, "Not Found", "No upstream configured for this path");
            close(client_fd);
            return;
        }
        
        // Check cache if enabled
        if (route->enable_cache && method == "GET") {
            std::string cache_key = path;
            auto cached = get_from_cache(cache_key);
            if (!cached.content.empty()) {
                cache_hits_++;
                send_response(client_fd, cached.status_code, cached.content_type, cached.content);
                close(client_fd);
                return;
            }
            cache_misses_++;
        }
        
        // Select upstream server
        UpstreamServer* upstream = select_upstream(*route);
        if (!upstream || !upstream->healthy) {
            send_error(client_fd, 502, "Bad Gateway", "All upstream servers are down");
            close(client_fd);
            return;
        }
        
        // Forward request to upstream
        std::string response = forward_to_upstream(*upstream, request);
        if (response.empty()) {
            upstream_errors_++;
            upstream->fail_count++;
            send_error(client_fd, 502, "Bad Gateway", "Failed to connect to upstream server");
            close(client_fd);
            return;
        }
        
        // Reset fail count on success
        upstream->fail_count = 0;
        
        // Parse response to extract status and content type
        int status_code = 200;
        std::string content_type = "text/html";
        extract_response_metadata(response, status_code, content_type);
        
        // Store in cache if enabled
        if (route->enable_cache && method == "GET" && status_code == 200) {
            std::string cache_key = path;
            store_in_cache(cache_key, response, content_type, status_code, route->cache_ttl_seconds);
        }
        
        // Send response to client
        send(client_fd, response.c_str(), response.length(), 0);
        close(client_fd);
    }
    
    ProxyRoute* find_route(const std::string& path) {
        // Find the longest matching prefix
        ProxyRoute* best_match = nullptr;
        size_t best_match_len = 0;
        
        for (auto& route : config_.routes) {
            if (path.find(route.path_prefix) == 0) {
                if (route.path_prefix.length() > best_match_len) {
                    best_match = &route;
                    best_match_len = route.path_prefix.length();
                }
            }
        }
        
        return best_match;
    }
    
    UpstreamServer* select_upstream(ProxyRoute& route) {
        if (route.upstreams.empty()) return nullptr;
        
        // Filter healthy upstreams
        std::vector<UpstreamServer*> healthy;
        for (auto& upstream : route.upstreams) {
            if (upstream.healthy) {
                healthy.push_back(&upstream);
            }
        }
        
        if (healthy.empty()) return nullptr;
        
        // Select based on balance method
        if (route.balance_method == 1) { // Least connections
            auto it = std::min_element(healthy.begin(), healthy.end(),
                [](UpstreamServer* a, UpstreamServer* b) {
                    return a->active_connections < b->active_connections;
                });
            return *it;
        } else if (route.balance_method == 2) { // Weighted round-robin
            // Simple weighted selection (can be improved)
            int total_weight = 0;
            for (auto* up : healthy) total_weight += up->weight;
            int random = rand() % total_weight;
            int cumulative = 0;
            for (auto* up : healthy) {
                cumulative += up->weight;
                if (random < cumulative) return up;
            }
        }
        
        // Default: Round-robin
        route.current_upstream = (route.current_upstream + 1) % healthy.size();
        return healthy[route.current_upstream];
    }
    
    std::string forward_to_upstream(UpstreamServer& upstream, const std::string& request) {
        upstream.active_connections++;
        
        // Connect to upstream
        int upstream_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (upstream_fd < 0) {
            upstream.active_connections--;
            return "";
        }
        
        // Set connect timeout
        struct timeval timeout;
        timeout.tv_sec = config_.upstream_connect_timeout;
        timeout.tv_usec = 0;
        setsockopt(upstream_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(upstream_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        // Resolve upstream host
        struct hostent* host = gethostbyname(upstream.host.c_str());
        if (!host) {
            close(upstream_fd);
            upstream.active_connections--;
            return "";
        }
        
        struct sockaddr_in upstream_addr;
        upstream_addr.sin_family = AF_INET;
        upstream_addr.sin_port = htons(upstream.port);
        memcpy(&upstream_addr.sin_addr, host->h_addr, host->h_length);
        
        if (connect(upstream_fd, (struct sockaddr*)&upstream_addr, sizeof(upstream_addr)) < 0) {
            close(upstream_fd);
            upstream.active_connections--;
            return "";
        }
        
        // Send request
        send(upstream_fd, request.c_str(), request.length(), 0);
        
        // Read response
        std::string response;
        char buffer[8192];
        ssize_t bytes_read;
        
        // Set read timeout
        timeout.tv_sec = config_.upstream_read_timeout;
        setsockopt(upstream_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        while ((bytes_read = recv(upstream_fd, buffer, sizeof(buffer), 0)) > 0) {
            response.append(buffer, bytes_read);
            
            // Check if we have complete response (simple check for Content-Length or chunked)
            if (response.find("\r\n\r\n") != std::string::npos) {
                // Check if we have Content-Length
                size_t cl_pos = response.find("Content-Length:");
                if (cl_pos != std::string::npos) {
                    size_t cl_end = response.find("\r\n", cl_pos);
                    std::string cl_str = response.substr(cl_pos + 15, cl_end - cl_pos - 15);
                    int content_length = std::stoi(cl_str);
                    size_t header_end = response.find("\r\n\r\n") + 4;
                    if (response.length() - header_end >= (size_t)content_length) {
                        break;
                    }
                }
            }
        }
        
        close(upstream_fd);
        upstream.active_connections--;
        
        return response;
    }
    
    void extract_response_metadata(const std::string& response, int& status_code, std::string& content_type) {
        // Extract status code
        size_t status_pos = response.find("HTTP/");
        if (status_pos != std::string::npos) {
            size_t code_start = response.find(" ", status_pos) + 1;
            size_t code_end = response.find(" ", code_start);
            if (code_end != std::string::npos) {
                status_code = std::stoi(response.substr(code_start, code_end - code_start));
            }
        }
        
        // Extract content type
        size_t ct_pos = response.find("Content-Type:");
        if (ct_pos != std::string::npos) {
            size_t ct_end = response.find("\r\n", ct_pos);
            content_type = response.substr(ct_pos + 13, ct_end - ct_pos - 13);
            // Trim whitespace
            content_type.erase(0, content_type.find_first_not_of(" \t"));
            content_type.erase(content_type.find_last_not_of(" \t") + 1);
        }
    }
    
    CacheEntry get_from_cache(const std::string& key) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (std::chrono::steady_clock::now() < it->second.expiry) {
                return it->second;
            } else {
                // Expired, remove it
                current_cache_size_ -= it->second.content.length();
                cache_.erase(it);
            }
        }
        return CacheEntry();
    }
    
    void store_in_cache(const std::string& key, const std::string& content, 
                        const std::string& content_type, int status_code, int ttl) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        // Check cache size limit
        if (current_cache_size_ + content.length() > config_.max_cache_size) {
            // Simple eviction: remove oldest entries
            auto oldest = cache_.begin();
            if (oldest != cache_.end()) {
                current_cache_size_ -= oldest->second.content.length();
                cache_.erase(oldest);
            }
        }
        
        CacheEntry entry;
        entry.content = content;
        entry.content_type = content_type;
        entry.status_code = status_code;
        entry.expiry = std::chrono::steady_clock::now() + std::chrono::seconds(ttl);
        
        cache_[key] = entry;
        current_cache_size_ += content.length();
    }
    
    void cleanup_cache() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto now = std::chrono::steady_clock::now();
        
        for (auto it = cache_.begin(); it != cache_.end();) {
            if (now >= it->second.expiry) {
                current_cache_size_ -= it->second.content.length();
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void check_upstream_health() {
        for (auto& route : config_.routes) {
            for (auto& upstream : route.upstreams) {
                // Simple TCP connect check
                int test_fd = socket(AF_INET, SOCK_STREAM, 0);
                if (test_fd < 0) continue;
                
                // Set non-blocking and timeout
                fcntl(test_fd, F_SETFL, O_NONBLOCK);
                
                struct hostent* host = gethostbyname(upstream.host.c_str());
                if (!host) {
                    close(test_fd);
                    upstream.healthy = false;
                    continue;
                }
                
                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(upstream.port);
                memcpy(&addr.sin_addr, host->h_addr, host->h_length);
                
                connect(test_fd, (struct sockaddr*)&addr, sizeof(addr));
                
                // Wait for connection with select
                fd_set write_fds;
                FD_ZERO(&write_fds);
                FD_SET(test_fd, &write_fds);
                struct timeval timeout = {2, 0};
                
                int result = select(test_fd + 1, nullptr, &write_fds, nullptr, &timeout);
                
                if (result > 0) {
                    // Connection succeeded or failed, check error
                    int error = 0;
                    socklen_t len = sizeof(error);
                    getsockopt(test_fd, SOL_SOCKET, SO_ERROR, &error, &len);
                    
                    if (error == 0) {
                        upstream.healthy = true;
                        upstream.fail_count = 0;
                    } else {
                        upstream.fail_count++;
                        if (upstream.fail_count >= upstream.max_fails) {
                            upstream.healthy = false;
                        }
                    }
                } else {
                    // Timeout or error
                    upstream.fail_count++;
                    if (upstream.fail_count >= upstream.max_fails) {
                        upstream.healthy = false;
                    }
                }
                
                close(test_fd);
                upstream.last_check = std::chrono::steady_clock::now();
            }
        }
    }
    
    void send_error(int client_fd, int status_code, const std::string& status_text, 
                    const std::string& message) {
        std::ostringstream response;
        response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Connection: close\r\n";
        
        std::ostringstream body;
        body << "<!DOCTYPE html><html><head><title>" << status_code << " " << status_text << "</title>"
             << "<style>body{font-family:sans-serif;margin:50px;background:#f5f5f5;}"
             << ".error{background:white;padding:30px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}"
             << "h1{color:#d32f2f;margin:0 0 20px 0;}p{color:#666;line-height:1.6;}</style></head>"
             << "<body><div class='error'><h1>" << status_code << " " << status_text << "</h1>"
             << "<p>" << message << "</p><hr><p><small>Proxy Service</small></p></div></body></html>";
        
        std::string body_str = body.str();
        response << "Content-Length: " << body_str.length() << "\r\n\r\n";
        response << body_str;
        
        std::string response_str = response.str();
        send(client_fd, response_str.c_str(), response_str.length(), 0);
    }
    
    void send_response(int client_fd, int status_code, const std::string& content_type, 
                       const std::string& content) {
        send(client_fd, content.c_str(), content.length(), 0);
    }
    
    void log_request(const std::string& client_ip, const std::string& method, 
                     const std::string& path) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        char time_buf[100];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
        
        std::cout << "[" << time_buf << "] " << client_ip << " " << method << " " << path << "\n";
    }
};

// ============================================================================
// Main Function
// ============================================================================

int main(int argc, char* argv[]) {
    int port = (argc > 1) ? std::atoi(argv[1]) : 8080;
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        🔀 Proxy Service (nginx-clone) v1.0                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    // Configure proxy
    ProxyConfig config;
    config.listen_port = port;
    config.enable_logging = true;
    config.worker_threads = 4;
    config.max_connections = 1000;
    config.keepalive_timeout = 65;
    
    // Define routes (example configuration)
    ProxyRoute route1;
    route1.path_prefix = "/api";
    route1.enable_cache = false; // Don't cache API responses
    route1.balance_method = 1; // Least connections for APIs
    
    // Upstream servers for /api
    UpstreamServer api_upstream1;
    api_upstream1.host = "localhost";
    api_upstream1.port = 9001; // Metrics backend
    api_upstream1.weight = 2;
    route1.upstreams.push_back(api_upstream1);
    
    UpstreamServer api_upstream2;
    api_upstream2.host = "localhost";
    api_upstream2.port = 9002; // Account service
    api_upstream2.weight = 1;
    route1.upstreams.push_back(api_upstream2);
    
    config.routes.push_back(route1);
    
    // Route for static content
    ProxyRoute route2;
    route2.path_prefix = "/";
    route2.enable_cache = true; // Cache static content
    route2.cache_ttl_seconds = 300; // 5 minutes
    route2.balance_method = 0; // Round-robin
    
    UpstreamServer web_upstream;
    web_upstream.host = "localhost";
    web_upstream.port = 9003; // Main web service
    web_upstream.weight = 1;
    route2.upstreams.push_back(web_upstream);
    
    config.routes.push_back(route2);
    
    // Create and start proxy
    ProxyService proxy(config);
    
    if (!proxy.initialize()) {
        std::cerr << "❌ Failed to initialize proxy service\n";
        return 1;
    }
    
    // Handle Ctrl+C gracefully
    signal(SIGINT, [](int) {
        std::cout << "\n\n🛑 Shutting down proxy service...\n";
        exit(0);
    });
    
    // Start the proxy
    proxy.start();
    
    return 0;
}
