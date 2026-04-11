#ifndef COOLBOX__LIBRARIES_BACKAGES_IO_QUIC_TRANSPORT_HEADERS_QUIC_TRANSPORT_HPP
#define COOLBOX__LIBRARIES_BACKAGES_IO_QUIC_TRANSPORT_HEADERS_QUIC_TRANSPORT_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "../../dataformats/http/headers/request_response.h"

namespace networking {
namespace quic {

enum class SessionRole {
    Client,
    Server
};

enum class SessionState {
    Idle,
    Handshaking,
    Active,
    Draining,
    Closed
};

struct TransportParameter {
    std::string name;
    std::string value;
};

struct TransportConfig {
    std::string application_protocol = "h3";
    std::size_t max_bidirectional_streams = 100;
    std::size_t max_unidirectional_streams = 16;
    std::size_t max_datagram_frame_size = 1200;
    bool enable_datagrams = true;
    bool verify_peer = false;
    std::vector<TransportParameter> parameters;
};

struct HeaderField {
    std::string name;
    std::string value;
};

struct Http3Request {
    std::uint64_t stream_id = 0;
    std::string method = "GET";
    std::string path = "/";
    std::vector<HeaderField> headers;
    std::string body;
};

struct Http3Response {
    std::uint64_t stream_id = 0;
    int status_code = 200;
    std::vector<HeaderField> headers;
    std::string body;
};

struct OutboundDatagram {
    std::uint64_t sequence = 0;
    std::uint64_t stream_id = 0;
    bool fin = false;
    std::string payload;
};

using Http3Handler = std::function<Http3Response(const Http3Request&)>;

class Session {
public:
    explicit Session(SessionRole role = SessionRole::Server,
                     TransportConfig config = {});

    const TransportConfig& config() const;
    SessionRole role() const;
    SessionState state() const;
    const std::string& peer_id() const;
    const std::string& close_reason() const;

    void begin_handshake(const std::string& peer_id);
    bool activate();
    bool is_ready() const;

    std::uint64_t open_stream(const std::string& purpose = "request");
    bool send_request(const Http3Request& request);
    bool send_response(const Http3Response& response);
    bool receive_datagram(const OutboundDatagram& datagram);

    bool has_outbound_datagrams() const;
    std::size_t outbound_datagram_count() const;
    OutboundDatagram pop_outbound_datagram();
    std::vector<OutboundDatagram> drain_outbound_datagrams();

    bool has_pending_requests() const;
    bool has_pending_responses() const;
    std::size_t pending_request_count() const;
    std::size_t pending_response_count() const;
    Http3Request pop_request();
    Http3Response pop_response();

    void close(const std::string& reason = "");

private:
    SessionRole role_;
    TransportConfig config_;
    SessionState state_ = SessionState::Idle;
    std::string peer_id_;
    std::string close_reason_;
    std::uint64_t next_stream_id_ = 0;
    std::uint64_t next_sequence_ = 1;
    std::queue<OutboundDatagram> outbound_datagrams_;
    std::queue<Http3Request> pending_requests_;
    std::queue<Http3Response> pending_responses_;
};

class RouteRegistry {
public:
    RouteRegistry& add_route(const std::string& path, Http3Handler handler);
    RouteRegistry& set_fallback(Http3Handler handler);

    bool has_route(const std::string& path) const;
    Http3Response dispatch(const Http3Request& request) const;

private:
    std::map<std::string, Http3Handler> routes_;
    Http3Handler fallback_;
};

class Endpoint {
public:
    Endpoint(std::string endpoint_id,
             SessionRole role,
             TransportConfig config = {});

    const std::string& endpoint_id() const;
    Session& session();
    const Session& session() const;

    void connect(const std::string& peer_id);
    std::uint64_t submit_request(Http3Request request);
    bool submit_response(Http3Response response);
    std::size_t flush_to(Endpoint& peer);
    std::size_t service(const RouteRegistry& routes);

    bool has_response() const;
    Http3Response pop_response();

private:
    std::string endpoint_id_;
    Session session_;
};

class LoopbackExchange {
public:
    explicit LoopbackExchange(TransportConfig config = {});

    Endpoint& client();
    Endpoint& server();
    RouteRegistry& routes();
    const RouteRegistry& routes() const;

    Http3Response perform(Http3Request request);

private:
    Endpoint client_;
    Endpoint server_;
    RouteRegistry routes_;
};

Http3Request request_from_http1(const Request& request,
                               std::uint64_t stream_id = 0);
Request request_to_http1(const Http3Request& request);
Http3Response response_from_http1(const Response& response,
                                  std::uint64_t stream_id = 0);
Response response_to_http1(const Http3Response& response);

} // namespace quic
} // namespace networking

#endif  // COOLBOX__LIBRARIES_BACKAGES_IO_QUIC_TRANSPORT_HEADERS_QUIC_TRANSPORT_HPP