#ifndef NETWORKING_REST_API_HTTP1_SERVLET_H
#define NETWORKING_REST_API_HTTP1_SERVLET_H

#include "networking/rest_api/http_server_base.h"

namespace networking {
namespace rest_api {

class Http1Servlet : public HttpServerBase {
private:
    struct Connection {
        std::string id;
        bool keep_alive;
        int requests_count;
        std::chrono::steady_clock::time_point last_activity;
        Connection() : keep_alive(true), requests_count(0), last_activity(std::chrono::steady_clock::now()) {}
    };
    std::map<std::string, Connection> connections_;
    std::mutex connections_mutex_;
public:
    Http1Servlet(int port = 8080, size_t num_threads = 4);
    ~Http1Servlet() override;

    HttpVersion protocol_version() const override { return HttpVersion::HTTP_1_1; }
    std::string protocol_name() const override { return "HTTP/1.1"; }
    ProtocolCapabilities capabilities() const override;
    void start() override;
    void stop() override;
    nhh::Response handle_request(const nhh::Request& request) override;
    void handle_request_async(const nhh::Request& request, std::function<void(const nhh::Response&)> callback) override;
    void set_keep_alive(bool enabled, int timeout = 5);
    void cleanup_idle_connections();

private:
    std::string generate_connection_id();
    void track_connection(const std::string& conn_id, const nhh::Request& request);
    bool should_keep_alive(const nhh::Request& request) const;
    void add_http1_headers(nhh::Response& response, bool keep_alive) const;
};

} // namespace rest_api
} // namespace networking

#endif // NETWORKING_REST_API_HTTP1_SERVLET_H
