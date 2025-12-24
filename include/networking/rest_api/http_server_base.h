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
#include <memory>

namespace ml {
namespace rest_api {

// Type aliases for convenience
using RouteHandler = std::function<Response(const Request&)>;
using Middleware = std::function<Response(const Request&, std::function<Response(const Request&)>)>;

// Abstract HTTP server interface
class HttpServerBase {
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
    virtual Response handle_request(const Request& request) = 0;
    virtual void handle_request_async(const Request& request, 
                                     std::function<void(const Response&)> callback) = 0;
    
    // Route management (common)
    void add_route(std::shared_ptr<Route> route) {
        routes_.push_back(route);
    }
    
    void get(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, HttpMethod::GET, handler));
    }
    
    void post(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, HttpMethod::POST, handler));
    }
    
    void put(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, HttpMethod::PUT, handler));
    }
    
    void delete_(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, HttpMethod::DELETE, handler));
    }
    
    void patch(const std::string& path, RouteHandler handler) {
        add_route(std::make_shared<Route>(path, HttpMethod::PATCH, handler));
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
    
protected:
    // Common request processing
    Response process_routes(const Request& request);
    Response apply_middleware(const Request& request, 
                             std::function<Response(const Request&)> handler);
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
} // namespace ml

#endif // NETWORKING_REST_API_HTTP_SERVER_BASE_H
