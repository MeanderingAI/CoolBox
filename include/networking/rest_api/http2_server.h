/**
 * HTTP/2 Server Implementation
 * 
 * Modern HTTP/2 server with:
 * - Multiplexing (multiple requests over single connection)
 * - Server push capabilities
 * - Header compression (HPACK)
 * - Stream prioritization
 * - Binary framing protocol
 */

#ifndef NETWORKING_REST_API_HTTP2_SERVER_H
#define NETWORKING_REST_API_HTTP2_SERVER_H

#include "http_server_base.h"
#include <unordered_map>

namespace ml {
namespace rest_api {

class Http2Server : public HttpServerBase {
private:
    // Connection management
    struct Http2Connection {
        std::string id;
        std::unordered_map<uint32_t, std::shared_ptr<Http2Stream>> streams;
        HPACKEncoder hpack_encoder;
        uint32_t next_stream_id;
        bool preface_received;
        std::chrono::steady_clock::time_point last_activity;
        
        Http2Connection() 
            : next_stream_id(1), 
              preface_received(false),
              last_activity(std::chrono::steady_clock::now()) {}
    };
    
    std::unordered_map<std::string, Http2Connection> connections_;
    std::mutex connections_mutex_;
    
public:
    Http2Server(int port = 8080, size_t num_threads = 4);
    ~Http2Server() override;
    
    // Protocol information
    HttpVersion protocol_version() const override { return HttpVersion::HTTP_2; }
    std::string protocol_name() const override { return "HTTP/2"; }
    ProtocolCapabilities capabilities() const override;
    
    // Server lifecycle
    void start() override;
    void stop() override;
    
    // Request handling
    Response handle_request(const Request& request) override;
    void handle_request_async(const Request& request, 
                             std::function<void(const Response&)> callback) override;
    
    // HTTP/2 specific features
    void enable_server_push(bool enabled = true);
    void set_max_concurrent_streams(uint32_t max);
    void set_initial_window_size(uint32_t size);
    
    // Server push
    void push_promise(const Request& original_request, 
                     const std::string& push_path,
                     const Response& push_response);
    
    // Stream management
    std::shared_ptr<Http2Stream> create_stream(const std::string& conn_id);
    void close_stream(const std::string& conn_id, uint32_t stream_id);
    
private:
    std::string generate_connection_id();
    Http2Frame encode_headers_frame(uint32_t stream_id, 
                                    const std::map<std::string, std::string>& headers);
    Http2Frame encode_data_frame(uint32_t stream_id, const std::vector<uint8_t>& data);
    Http2Frame encode_settings_frame();
    
    void handle_frame(const std::string& conn_id, const Http2Frame& frame);
    void handle_settings_frame(const std::string& conn_id, const Http2Frame& frame);
    void handle_headers_frame(const std::string& conn_id, const Http2Frame& frame);
    void handle_data_frame(const std::string& conn_id, const Http2Frame& frame);
    
    void send_connection_preface(const std::string& conn_id);
    void update_window_size(const std::string& conn_id, uint32_t stream_id, int32_t delta);
};

} // namespace rest_api
} // namespace ml

#endif // NETWORKING_REST_API_HTTP2_SERVER_H
