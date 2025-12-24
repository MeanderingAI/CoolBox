/**
 * HTTP/1.1 Server Implementation
 * 
 * Traditional HTTP/1.1 server with:
 * - Persistent connections (Keep-Alive)
 * - Pipelining support
 * - Chunked transfer encoding
 */

#ifndef NETWORKING_REST_API_HTTP1_SERVER_H
#define NETWORKING_REST_API_HTTP1_SERVER_H

#include "http_server_base.h"

namespace ml {
namespace rest_api {

class Http1Server : public HttpServerBase {
private:
    // Connection tracking
    struct Connection {
        std::string id;
        bool keep_alive;
        int requests_count;
        std::chrono::steady_clock::time_point last_activity;
        
        Connection() : keep_alive(true), requests_count(0), 
                      last_activity(std::chrono::steady_clock::now()) {}
    };
    
    std::map<std::string, Connection> connections_;
    std::mutex connections_mutex_;
    
public:
    Http1Server(int port = 8080, size_t num_threads = 4);
    ~Http1Server() override;
    
    // Protocol information
    HttpVersion protocol_version() const override { return HttpVersion::HTTP_1_1; }
    std::string protocol_name() const override { return "HTTP/1.1"; }
    ProtocolCapabilities capabilities() const override;
    
    // Server lifecycle
    void start() override;
    void stop() override;
    
    // Request handling
    Response handle_request(const Request& request) override;
    void handle_request_async(const Request& request, 
                             std::function<void(const Response&)> callback) override;
    
    // HTTP/1.1 specific
    void set_keep_alive(bool enabled, int timeout = 5);
    void cleanup_idle_connections();
    
private:
    std::string generate_connection_id();
    void track_connection(const std::string& conn_id, const Request& request);
    bool should_keep_alive(const Request& request) const;
    void add_http1_headers(Response& response, bool keep_alive) const;
};

} // namespace rest_api
} // namespace ml

#endif // NETWORKING_REST_API_HTTP1_SERVER_H
