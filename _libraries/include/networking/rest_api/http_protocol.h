/**
 * HTTP Protocol Definitions
 * 
 * Defines HTTP protocol versions and common interfaces
 * for HTTP/1.1, HTTP/2, and HTTP/3 implementations.
 */

#ifndef NETWORKING_REST_API_HTTP_PROTOCOL_H
#define NETWORKING_REST_API_HTTP_PROTOCOL_H

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace networking {
namespace rest_api {

// HTTP protocol versions
enum class HttpVersion {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2,
    HTTP_3
};

// Convert version enum to string
std::string http_version_to_string(HttpVersion version);

// HTTP/2 specific features
struct Http2Frame {
    enum class Type {
        DATA,
        HEADERS,
        PRIORITY,
        RST_STREAM,
        SETTINGS,
        PUSH_PROMISE,
        PING,
        GOAWAY,
        WINDOW_UPDATE,
        CONTINUATION
    };
    
    Type type;
    uint32_t stream_id;
    std::vector<uint8_t> payload;
    uint8_t flags;
};

// HTTP/2 stream state
enum class Http2StreamState {
    IDLE,
    OPEN,
    RESERVED_LOCAL,
    RESERVED_REMOTE,
    HALF_CLOSED_LOCAL,
    HALF_CLOSED_REMOTE,
    CLOSED
};

// HTTP/2 stream
struct Http2Stream {
    uint32_t id;
    Http2StreamState state;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> data;
    int32_t window_size;
    uint8_t priority;
    
    Http2Stream(uint32_t stream_id) 
        : id(stream_id), 
          state(Http2StreamState::IDLE),
          window_size(65535),
          priority(0) {}
};

// HTTP/3 QUIC connection
struct Http3Connection {
    std::string connection_id;
    std::map<uint64_t, std::shared_ptr<Http2Stream>> streams;  // Reuse stream structure
    bool is_established;
    uint64_t next_stream_id;
    
    Http3Connection() 
        : is_established(false),
          next_stream_id(0) {}
};

// HPACK encoder/decoder for HTTP/2 header compression
class HPACKEncoder {
private:
    std::map<std::string, std::string> dynamic_table_;
    size_t table_size_ = 4096;
    
public:
    std::vector<uint8_t> encode(const std::map<std::string, std::string>& headers);
    std::map<std::string, std::string> decode(const std::vector<uint8_t>& data);
    
    void set_table_size(size_t size) { table_size_ = size; }
};

// QPACK encoder/decoder for HTTP/3 header compression
class QPACKEncoder {
private:
    std::map<std::string, std::string> dynamic_table_;
    size_t table_size_ = 4096;
    
public:
    std::vector<uint8_t> encode(const std::map<std::string, std::string>& headers);
    std::map<std::string, std::string> decode(const std::vector<uint8_t>& data);
    
    void set_table_size(size_t size) { table_size_ = size; }
};

// Protocol-specific settings
struct ProtocolSettings {
    HttpVersion version;
    
    // HTTP/1.1 settings
    bool keep_alive = true;
    int keep_alive_timeout = 5;
    
    // HTTP/2 settings
    bool enable_push = false;
    uint32_t max_concurrent_streams = 100;
    uint32_t initial_window_size = 65535;
    uint32_t max_frame_size = 16384;
    uint32_t max_header_list_size = 8192;
    
    // HTTP/3 settings
    uint64_t max_idle_timeout = 30000;  // ms
    uint64_t max_udp_payload_size = 1200;
    bool enable_0rtt = false;
    
    ProtocolSettings(HttpVersion v = HttpVersion::HTTP_1_1) : version(v) {}
};

// Protocol capabilities
struct ProtocolCapabilities {
    bool supports_multiplexing;
    bool supports_server_push;
    bool supports_header_compression;
    bool supports_prioritization;
    bool is_encrypted;
    bool is_udp_based;
    
    static ProtocolCapabilities for_version(HttpVersion version);
};


} // namespace rest_api
} // namespace networking

#endif // NETWORKING_REST_API_HTTP_PROTOCOL_H
