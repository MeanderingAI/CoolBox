/**
 * HTTP Server Base Class
 * 
 * Abstract base class for HTTP servers supporting different protocol versions.
 * Provides common interface for HTTP/1.1, HTTP/2, and HTTP/3 implementations.
 */

#ifndef NETWORKING_REST_API_HTTP_SERVER_BASE_H
#define NETWORKING_REST_API_HTTP_SERVER_BASE_H

#include "server.h"
#include "http_protocol.h"
#include "networking/http/request_response.h"
#include <memory>

namespace nhh = networking::http;
namespace networking {
namespace rest_api {

// Type aliases for convenience
using RouteHandler = std::function<nhh::Response(const nhh::Request&)>;
using Middleware = std::function<nhh::Response(const nhh::Request&, std::function<nhh::Response(const nhh::Request&)>)>;

// Abstract HTTP server interface
class HttpServerBase {
public:
    // Hot reload support: called when a watched file is modified
    virtual void reload_file(const std::string& path) {
        // Default: do nothing. Derived classes can override if needed.
    }
    // Load multiple routes from a vector of Route objects
    void load_routes(const std::vector<std::shared_ptr<Route>>& routes) {
        for (const auto& r : routes) {
            add_route(r);
        }
    }
protected:
    int port_;
    size_t num_threads_;
    bool running_;
    ProtocolSettings settings_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::vector<std::shared_ptr<Route>> routes_;
    std::vector<Middleware> middleware_;
    bool cors_enabled_;
    std::string cors_origin_;
    
public:
    HttpServerBase(int port, size_t num_threads, HttpVersion version)
        : port_(port), 
          num_threads_(num_threads), 
          running_(false),
          settings_(version),
          thread_pool_(std::make_unique<ThreadPool>(num_threads)),
          cors_enabled_(false),
          cors_origin_("*") {}
    
    virtual ~HttpServerBase() = default;
    
    // Protocol information
    virtual HttpVersion protocol_version() const = 0;
    virtual std::string protocol_name() const = 0;
    virtual ProtocolCapabilities capabilities() const = 0;
    
    // Server lifecycle
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const { return running_; }
    
    // Request handling (protocol-specific)
    virtual nhh::Response handle_request(const nhh::Request& request) = 0;
    virtual void handle_request_async(const nhh::Request& request, 
                                     std::function<void(const nhh::Response&)> callback) = 0;
    
    // Route management (common)
    void add_route(std::shared_ptr<Route> route) {
        routes_.push_back(route);
    }
    
    void get(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, nhh::HttpMethod::GET, handler));
    }
    
    void post(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, nhh::HttpMethod::POST, handler));
    }
    
    void put(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, nhh::HttpMethod::PUT, handler));
    }
    
    void delete_(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, nhh::HttpMethod::DELETE_, handler));
    }
    
    void patch(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, nhh::HttpMethod::PATCH, handler));
    }
    
    // Middleware
    void use(Middleware middleware) {
        middleware_.push_back(middleware);
    }
    
    // CORS
    void enable_cors(const std::string& origin = "*") {
        cors_enabled_ = true;
        cors_origin_ = origin;
    }
    
    // Settings
    ProtocolSettings& settings() { return settings_; }
    const ProtocolSettings& settings() const { return settings_; }
    
    int port() const { return port_; }
    size_t num_threads() const { return num_threads_; }

    // List all registered routes (method, pattern)
    std::vector<std::pair<std::string, std::string>> list_routes() const {
        std::vector<std::pair<std::string, std::string>> out;
        for (const auto& route : routes_) {
            std::string method;
            switch (route->method()) {
                case nhh::HttpMethod::GET: method = "GET"; break;
                case nhh::HttpMethod::POST: method = "POST"; break;
                case nhh::HttpMethod::PUT: method = "PUT"; break;
                case nhh::HttpMethod::DELETE_: method = "DELETE"; break;
                case nhh::HttpMethod::PATCH: method = "PATCH"; break;
                case nhh::HttpMethod::OPTIONS: method = "OPTIONS"; break;
                default: method = "UNKNOWN";
            }
            out.emplace_back(method, route->pattern());
        }
        return out;
    }
    
protected:
    // Common request processing
    nhh::Response process_routes(const nhh::Request& request);
    nhh::Response apply_middleware(const nhh::Request& request, 
                             std::function<nhh::Response(const nhh::Request&)> handler);
};

// Factory for creating HTTP servers
class HttpServerFactory {
public:
    static std::unique_ptr<HttpServerBase> create(
        HttpVersion version,
        int port = 8080,
        size_t num_threads = 4
    );
    
    static std::unique_ptr<HttpServerBase> create_http1(int port = 8080, size_t num_threads = 4);
    static std::unique_ptr<HttpServerBase> create_http2(int port = 8080, size_t num_threads = 4);
    static std::unique_ptr<HttpServerBase> create_http3(int port = 8080, size_t num_threads = 4);
};

} // namespace rest_api
} // namespace networking

#endif // NETWORKING_REST_API_HTTP_SERVER_BASE_H
