/**
 * HTTP/3 Server Implementation
 * 
 * Next-generation HTTP/3 server with:
 * - QUIC protocol (UDP-based)
 * - 0-RTT connection establishment
 * - Improved loss recovery
 * - Header compression (QPACK)
 * - No head-of-line blocking
 * - Built-in encryption (TLS 1.3)
 */

#ifndef NETWORKING_REST_API_HTTP3_SERVER_H
#define NETWORKING_REST_API_HTTP3_SERVER_H

#include "http_server_base.h"
#include <unordered_map>


namespace nhh = networking::http;
namespace networking {
namespace rest_api {

class Http3Server : public HttpServerBase {
private:
    // QUIC connection tracking
    struct QuicConnection {
        std::string connection_id;
        std::unordered_map<uint64_t, std::shared_ptr<Http2Stream>> streams;  // Reuse stream structure
        QPACKEncoder qpack_encoder;
        uint64_t next_stream_id;
        bool is_established;
        std::chrono::steady_clock::time_point established_time;
        std::chrono::steady_clock::time_point last_activity;
        
        // QUIC-specific
        uint64_t packet_number;
        std::vector<uint8_t> initial_secret;
        bool zero_rtt_enabled;
        
        QuicConnection() 
            : next_stream_id(0), 
              is_established(false),
              established_time(std::chrono::steady_clock::now()),
              last_activity(std::chrono::steady_clock::now()),
              packet_number(0),
              zero_rtt_enabled(false) {}
    };
    
    std::unordered_map<std::string, QuicConnection> connections_;
    std::mutex connections_mutex_;
    
public:
    Http3Server(int port = 8080, size_t num_threads = 4);
    ~Http3Server() override;
    
    // Protocol information
    HttpVersion protocol_version() const override { return HttpVersion::HTTP_3; }
    std::string protocol_name() const override { return "HTTP/3"; }
    ProtocolCapabilities capabilities() const override;
    
    // Server lifecycle
    void start() override;
    void stop() override;
    
    // Request handling
    nhh::Response handle_request(const nhh::Request& request) override;
    void handle_request_async(const nhh::Request& request, 
                             std::function<void(const nhh::Response&)> callback) override;
    
    // HTTP/3 specific features
    void enable_0rtt(bool enabled = true);
    void set_max_idle_timeout(uint64_t ms);
    void set_max_udp_payload_size(uint64_t size);
    
    // QUIC connection management
    std::string create_connection(const std::string& client_id);
    void close_connection(const std::string& conn_id, uint64_t error_code = 0);
    bool is_connection_established(const std::string& conn_id) const;
    
    // Stream management
    std::shared_ptr<Http2Stream> create_stream(const std::string& conn_id);
    void close_stream(const std::string& conn_id, uint64_t stream_id);
    
private:
    std::string generate_connection_id();
    std::vector<uint8_t> generate_initial_secret();
    
    // QUIC packet handling
    struct QuicPacket {
        uint64_t packet_number;
        std::vector<uint8_t> payload;
        bool is_initial;
        bool is_0rtt;
    };
    
    QuicPacket create_packet(const std::string& conn_id, const std::vector<uint8_t>& data);
    void handle_packet(const std::string& conn_id, const QuicPacket& packet);
    
    // Frame handling
    void handle_stream_frame(const std::string& conn_id, uint64_t stream_id, 
                            const std::vector<uint8_t>& data);
    void handle_connection_close(const std::string& conn_id, uint64_t error_code);
    
    // Header compression
    std::vector<uint8_t> encode_headers(const std::string& conn_id, 
                                       const std::map<std::string, std::string>& headers);
    std::map<std::string, std::string> decode_headers(const std::string& conn_id, 
                                                      const std::vector<uint8_t>& data);
    
    // Connection cleanup
    void cleanup_idle_connections();
};


} // namespace rest_api
} // namespace networking

#endif // NETWORKING_REST_API_HTTP3_SERVER_H
