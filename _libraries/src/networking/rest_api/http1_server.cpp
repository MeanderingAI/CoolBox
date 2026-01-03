#include "advanced_logging/advanced_logging.h"
static advanced_logging::Logger http1_logger("http1_server.log");
#include "networking/rest_api/http1_server.h"
#include <iostream>
#include <random>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace networking {
namespace rest_api {

// =========================================================================
// Http1Server Implementation
// =========================================================================

Http1Server::Http1Server(int port, size_t num_threads)
    : HttpServerBase(port, num_threads, HttpVersion::HTTP_1_1) {
    settings_.keep_alive = true;
    settings_.keep_alive_timeout = 5;
}

Http1Server::~Http1Server() {
    stop();
}

ProtocolCapabilities Http1Server::capabilities() const {
    return ProtocolCapabilities::for_version(HttpVersion::HTTP_1_1);
}

void Http1Server::start() {
    running_ = true;
    std::cout << "HTTP/1.1 Server started on port " << port_ << std::endl;
    std::cout << "Thread pool size: " << num_threads_ << " threads" << std::endl;
    std::cout << "Keep-Alive: " << (settings_.keep_alive ? "enabled" : "disabled");
    if (settings_.keep_alive) {
        std::cout << " (timeout: " << settings_.keep_alive_timeout << "s)";
    }
    std::cout << std::endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        running_ = false;
        return;
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        running_ = false;
        return;
    }
    if (listen(server_fd, 16) < 0) {
        perror("listen");
        close(server_fd);
        running_ = false;
        return;
    }
    std::cout << "Listening for HTTP/1.1 connections on port " << port_ << std::endl;
    while (running_) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            if (running_) perror("accept");
            continue;
        }
        // Read request (very basic, not production-ready)
        char buffer[4096] = {0};
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            std::string req_str(buffer, n);
            // Parse method and path (very basic)
            std::istringstream iss(req_str);
            std::string method, path, version;
            iss >> method >> path >> version;
            networking::http::HttpMethod method_enum = networking::http::HttpMethod::GET;
            if (method == "GET") method_enum = networking::http::HttpMethod::GET;
            else if (method == "POST") method_enum = networking::http::HttpMethod::POST;
            else if (method == "PUT") method_enum = networking::http::HttpMethod::PUT;
            else if (method == "DELETE") method_enum = networking::http::HttpMethod::DELETE_;
            else if (method == "PATCH") method_enum = networking::http::HttpMethod::PATCH;
            else if (method == "OPTIONS") method_enum = networking::http::HttpMethod::OPTIONS;
            networking::http::Request req(method_enum, path, {}, "");
            nhh::Response resp = handle_request(req);
            std::string resp_str = resp.to_string();
            write(client_fd, resp_str.c_str(), resp_str.size());
        }
        close(client_fd);
    }
    close(server_fd);
}

void Http1Server::stop() {
    running_ = false;
    if (thread_pool_) {
        thread_pool_->stop();
    }
    std::cout << "HTTP/1.1 Server stopped" << std::endl;
}

nhh::Response Http1Server::handle_request(const nhh::Request& request) {
    // Track connection
    std::string conn_id = generate_connection_id();
    track_connection(conn_id, request);
    
    // Process request through routes
    nhh::Response response = process_routes(request);
    
    // Add HTTP/1.1 specific headers
    bool keep_alive = should_keep_alive(request);
    add_http1_headers(response, keep_alive);
    
    return response;
}

void Http1Server::handle_request_async(const nhh::Request& request,
                                       std::function<void(const nhh::Response&)> callback) {
    thread_pool_->enqueue([this, request, callback]() {
        nhh::Response response = handle_request(request);
        if (callback) {
            callback(response);
        }
    });
}

void Http1Server::set_keep_alive(bool enabled, int timeout) {
    settings_.keep_alive = enabled;
    settings_.keep_alive_timeout = timeout;
}

void Http1Server::cleanup_idle_connections() {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto it = connections_.begin();
    
    while (it != connections_.end()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.last_activity).count();
        
        if (elapsed > settings_.keep_alive_timeout) {
            it = connections_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string Http1Server::generate_connection_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 999999);
    
    std::ostringstream oss;
    oss << "conn_" << dis(gen);
    return oss.str();
}

void Http1Server::track_connection(const std::string& conn_id, const nhh::Request& request) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto& conn = connections_[conn_id];
    conn.id = conn_id;
    conn.requests_count++;
    conn.last_activity = std::chrono::steady_clock::now();
    conn.keep_alive = should_keep_alive(request);
}

bool Http1Server::should_keep_alive(const nhh::Request& request) const {
    if (!settings_.keep_alive) {
        return false;
    }
    
    // Check Connection header
    auto headers = request.headers();
    auto it = headers.find("Connection");
    if (it != headers.end()) {
        return it->second != "close";
    }
    
    return true;
}

void Http1Server::add_http1_headers(nhh::Response& response, bool keep_alive) const {
    // Set HTTP version
    response.set_header("Server", "ToolBox/1.0 (HTTP/1.1)");
    
    // Connection header
    if (keep_alive) {
        response.set_header("Connection", "keep-alive");
        response.set_header("Keep-Alive", 
            "timeout=" + std::to_string(settings_.keep_alive_timeout));
    } else {
        response.set_header("Connection", "close");
    }
    
    // Content-Length
    response.set_header("Content-Length", std::to_string(response.body().length()));
}

} // namespace rest_api
} // namespace ml
