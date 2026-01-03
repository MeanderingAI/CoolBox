#include "advanced_logging/advanced_logging.h"
static advanced_logging::Logger http2_logger("http2_server.log");
#include "networking/rest_api/http2_server.h"
#include <iostream>
#include <random>
#include <sstream>

namespace networking {
namespace rest_api {

// =========================================================================
// Http2Server Implementation
// =========================================================================

Http2Server::Http2Server(int port, size_t num_threads)
    : HttpServerBase(port, num_threads, HttpVersion::HTTP_2) {
    settings_.enable_push = false;
    settings_.max_concurrent_streams = 100;
    settings_.initial_window_size = 65535;
    settings_.max_frame_size = 16384;
    settings_.max_header_list_size = 8192;
}

Http2Server::~Http2Server() {
    stop();
}

ProtocolCapabilities Http2Server::capabilities() const {
    return ProtocolCapabilities::for_version(HttpVersion::HTTP_2);
}

void Http2Server::start() {
    running_ = true;
    std::cout << "HTTP/2 Server started on port " << port_ << std::endl;
    std::cout << "Protocol: HTTP/2 (binary framing, multiplexing enabled)" << std::endl;
    std::cout << "Thread pool size: " << num_threads_ << " threads" << std::endl;
    std::cout << "Max concurrent streams: " << settings_.max_concurrent_streams << std::endl;
    std::cout << "Server push: " << (settings_.enable_push ? "enabled" : "disabled") << std::endl;
    std::cout << "Header compression: HPACK" << std::endl;
}

void Http2Server::stop() {
    running_ = false;
    if (thread_pool_) {
        thread_pool_->stop();
    }
    std::cout << "HTTP/2 Server stopped" << std::endl;
}

nhh::Response Http2Server::handle_request(const nhh::Request& request) {
    // Create or get connection
    std::string conn_id = generate_connection_id();
    
    // Create stream for this request
    auto stream = create_stream(conn_id);
    
    // Process request through routes
    nhh::Response response = process_routes(request);
    
    // Add HTTP/2 specific headers
    response.set_header("Server", "ToolBox/1.0 (HTTP/2)");
    
    // Close stream after response
    close_stream(conn_id, stream->id);
    
    return response;
}

void Http2Server::handle_request_async(const nhh::Request& request,
                                       std::function<void(const nhh::Response&)> callback) {
    thread_pool_->enqueue([this, request, callback]() {
        nhh::Response response = handle_request(request);
        if (callback) {
            callback(response);
        }
    });
}

void Http2Server::enable_server_push(bool enabled) {
    settings_.enable_push = enabled;
}

void Http2Server::set_max_concurrent_streams(uint32_t max) {
    settings_.max_concurrent_streams = max;
}

void Http2Server::set_initial_window_size(uint32_t size) {
    settings_.initial_window_size = size;
}

void Http2Server::push_promise(const nhh::Request& original_request,
                               const std::string& push_path,
                               const nhh::Response& push_response) {
    if (!settings_.enable_push) {
        return;
    }
    
    std::cout << "Server push: " << push_path << std::endl;
    // In a real implementation, this would send PUSH_PROMISE frame
}

std::shared_ptr<Http2Stream> Http2Server::create_stream(const std::string& conn_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto& conn = connections_[conn_id];
    uint32_t stream_id = conn.next_stream_id;
    conn.next_stream_id += 2;  // Client streams are odd, server streams are even
    
    auto stream = std::make_shared<Http2Stream>(stream_id);
    stream->state = Http2StreamState::OPEN;
    stream->window_size = settings_.initial_window_size;
    
    conn.streams[stream_id] = stream;
    
    return stream;
}

void Http2Server::close_stream(const std::string& conn_id, uint32_t stream_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto conn_it = connections_.find(conn_id);
    if (conn_it != connections_.end()) {
        auto stream_it = conn_it->second.streams.find(stream_id);
        if (stream_it != conn_it->second.streams.end()) {
            stream_it->second->state = Http2StreamState::CLOSED;
            conn_it->second.streams.erase(stream_it);
        }
    }
}

std::string Http2Server::generate_connection_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 999999);
    
    std::ostringstream oss;
    oss << "h2_conn_" << dis(gen);
    return oss.str();
}

Http2Frame Http2Server::encode_headers_frame(uint32_t stream_id,
                                             const std::map<std::string, std::string>& headers) {
    Http2Frame frame;
    frame.type = Http2Frame::Type::HEADERS;
    frame.stream_id = stream_id;
    frame.flags = 0x04;  // END_HEADERS
    
    // Encode headers using HPACK
    std::lock_guard<std::mutex> lock(connections_mutex_);
    // Simplified - would use connection's HPACK encoder
    HPACKEncoder encoder;
    frame.payload = encoder.encode(headers);
    
    return frame;
}

Http2Frame Http2Server::encode_data_frame(uint32_t stream_id, const std::vector<uint8_t>& data) {
    Http2Frame frame;
    frame.type = Http2Frame::Type::DATA;
    frame.stream_id = stream_id;
    frame.flags = 0x01;  // END_STREAM
    frame.payload = data;
    
    return frame;
}

Http2Frame Http2Server::encode_settings_frame() {
    Http2Frame frame;
    frame.type = Http2Frame::Type::SETTINGS;
    frame.stream_id = 0;
    frame.flags = 0;
    
    // Encode settings
    // Format: id (2 bytes) + value (4 bytes)
    // This is simplified
    
    return frame;
}

void Http2Server::handle_frame(const std::string& conn_id, const Http2Frame& frame) {
    switch (frame.type) {
        case Http2Frame::Type::SETTINGS:
            handle_settings_frame(conn_id, frame);
            break;
        case Http2Frame::Type::HEADERS:
            handle_headers_frame(conn_id, frame);
            break;
        case Http2Frame::Type::DATA:
            handle_data_frame(conn_id, frame);
            break;
        default:
            break;
    }
}

void Http2Server::handle_settings_frame(const std::string& conn_id, const Http2Frame& frame) {
    // Process settings and send ACK
}

void Http2Server::handle_headers_frame(const std::string& conn_id, const Http2Frame& frame) {
    // Decode HPACK headers
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto& conn = connections_[conn_id];
    auto headers = conn.hpack_encoder.decode(frame.payload);
    
    // Store in stream
    auto it = conn.streams.find(frame.stream_id);
    if (it != conn.streams.end()) {
        it->second->headers = headers;
    }
}

void Http2Server::handle_data_frame(const std::string& conn_id, const Http2Frame& frame) {
    // Store data in stream
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto conn_it = connections_.find(conn_id);
    if (conn_it != connections_.end()) {
        auto stream_it = conn_it->second.streams.find(frame.stream_id);
        if (stream_it != conn_it->second.streams.end()) {
            stream_it->second->data = frame.payload;
        }
    }
}

void Http2Server::send_connection_preface(const std::string& conn_id) {
    // Send connection preface: PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n
    // Followed by SETTINGS frame
}

void Http2Server::update_window_size(const std::string& conn_id, uint32_t stream_id, int32_t delta) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto conn_it = connections_.find(conn_id);
    if (conn_it != connections_.end()) {
        auto stream_it = conn_it->second.streams.find(stream_id);
        if (stream_it != conn_it->second.streams.end()) {
            stream_it->second->window_size += delta;
        }
    }
}

} // namespace rest_api
} // namespace ml
