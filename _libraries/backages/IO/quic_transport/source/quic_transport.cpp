#include "quic_transport.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace networking {
namespace quic {
namespace {

std::string encode_headers(const std::vector<HeaderField>& headers) {
    std::ostringstream stream;
    stream << headers.size() << '\n';
    for (const auto& header : headers) {
        stream << header.name << ':' << header.value << '\n';
    }
    return stream.str();
}

std::vector<HeaderField> decode_headers(std::istream& stream) {
    std::size_t count = 0;
    stream >> count;
    stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::vector<HeaderField> headers;
    headers.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        std::string line;
        std::getline(stream, line);
        const std::size_t separator = line.find(':');
        if (separator == std::string::npos) {
            headers.push_back({line, ""});
            continue;
        }
        headers.push_back({line.substr(0, separator), line.substr(separator + 1)});
    }
    return headers;
}

OutboundDatagram make_datagram(std::uint64_t sequence,
                               std::uint64_t stream_id,
                               bool fin,
                               const std::string& kind,
                               const std::string& payload) {
    OutboundDatagram datagram;
    datagram.sequence = sequence;
    datagram.stream_id = stream_id;
    datagram.fin = fin;
    datagram.payload = kind + "\n" + payload;
    return datagram;
}

} // namespace

Session::Session(SessionRole role, TransportConfig config)
    : role_(role), config_(std::move(config)) {}

const TransportConfig& Session::config() const {
    return config_;
}

SessionRole Session::role() const {
    return role_;
}

SessionState Session::state() const {
    return state_;
}

const std::string& Session::peer_id() const {
    return peer_id_;
}

const std::string& Session::close_reason() const {
    return close_reason_;
}

void Session::begin_handshake(const std::string& peer_id) {
    peer_id_ = peer_id;
    state_ = SessionState::Handshaking;
}

bool Session::activate() {
    if (state_ == SessionState::Closed) {
        return false;
    }
    if (state_ == SessionState::Idle) {
        begin_handshake("loopback");
    }
    state_ = SessionState::Active;
    return true;
}

bool Session::is_ready() const {
    return state_ == SessionState::Active;
}

std::uint64_t Session::open_stream(const std::string& purpose) {
    (void)purpose;
    if (!is_ready()) {
        activate();
    }
    const std::uint64_t stream_id = next_stream_id_;
    next_stream_id_ += 4;
    return stream_id;
}

bool Session::send_request(const Http3Request& request) {
    if (!is_ready()) {
        activate();
    }
    std::ostringstream payload;
    payload << request.stream_id << '\n'
            << request.method << '\n'
            << request.path << '\n'
            << encode_headers(request.headers)
            << request.body;
    outbound_datagrams_.push(make_datagram(next_sequence_++, request.stream_id, true, "REQ", payload.str()));
    return true;
}

bool Session::send_response(const Http3Response& response) {
    if (!is_ready()) {
        activate();
    }
    std::ostringstream payload;
    payload << response.stream_id << '\n'
            << response.status_code << '\n'
            << encode_headers(response.headers)
            << response.body;
    outbound_datagrams_.push(make_datagram(next_sequence_++, response.stream_id, true, "RES", payload.str()));
    return true;
}

bool Session::receive_datagram(const OutboundDatagram& datagram) {
    std::istringstream stream(datagram.payload);
    std::string kind;
    std::getline(stream, kind);
    if (kind == "REQ") {
        Http3Request request;
        stream >> request.stream_id;
        stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(stream, request.method);
        std::getline(stream, request.path);
        request.headers = decode_headers(stream);
        std::ostringstream body;
        body << stream.rdbuf();
        request.body = body.str();
        pending_requests_.push(std::move(request));
        return true;
    }
    if (kind == "RES") {
        Http3Response response;
        stream >> response.stream_id;
        stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        stream >> response.status_code;
        stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        response.headers = decode_headers(stream);
        std::ostringstream body;
        body << stream.rdbuf();
        response.body = body.str();
        pending_responses_.push(std::move(response));
        return true;
    }
    return false;
}

bool Session::has_outbound_datagrams() const {
    return !outbound_datagrams_.empty();
}

std::size_t Session::outbound_datagram_count() const {
    return outbound_datagrams_.size();
}

OutboundDatagram Session::pop_outbound_datagram() {
    if (outbound_datagrams_.empty()) {
        throw std::runtime_error("No outbound QUIC datagrams are available");
    }
    OutboundDatagram datagram = outbound_datagrams_.front();
    outbound_datagrams_.pop();
    return datagram;
}

std::vector<OutboundDatagram> Session::drain_outbound_datagrams() {
    std::vector<OutboundDatagram> datagrams;
    datagrams.reserve(outbound_datagrams_.size());
    while (!outbound_datagrams_.empty()) {
        datagrams.push_back(outbound_datagrams_.front());
        outbound_datagrams_.pop();
    }
    return datagrams;
}

bool Session::has_pending_requests() const {
    return !pending_requests_.empty();
}

bool Session::has_pending_responses() const {
    return !pending_responses_.empty();
}

std::size_t Session::pending_request_count() const {
    return pending_requests_.size();
}

std::size_t Session::pending_response_count() const {
    return pending_responses_.size();
}

Http3Request Session::pop_request() {
    if (pending_requests_.empty()) {
        throw std::runtime_error("No pending HTTP/3 request is available");
    }
    Http3Request request = pending_requests_.front();
    pending_requests_.pop();
    return request;
}

Http3Response Session::pop_response() {
    if (pending_responses_.empty()) {
        throw std::runtime_error("No pending HTTP/3 response is available");
    }
    Http3Response response = pending_responses_.front();
    pending_responses_.pop();
    return response;
}

void Session::close(const std::string& reason) {
    close_reason_ = reason;
    state_ = SessionState::Closed;
}

RouteRegistry& RouteRegistry::add_route(const std::string& path, Http3Handler handler) {
    routes_[path] = std::move(handler);
    return *this;
}

RouteRegistry& RouteRegistry::set_fallback(Http3Handler handler) {
    fallback_ = std::move(handler);
    return *this;
}

bool RouteRegistry::has_route(const std::string& path) const {
    return routes_.find(path) != routes_.end();
}

Http3Response RouteRegistry::dispatch(const Http3Request& request) const {
    const auto route = routes_.find(request.path);
    if (route != routes_.end()) {
        return route->second(request);
    }
    if (fallback_) {
        return fallback_(request);
    }

    Http3Response response;
    response.stream_id = request.stream_id;
    response.status_code = 404;
    response.headers.push_back({"content-type", "text/plain"});
    response.body = "No HTTP/3 route is registered for " + request.path;
    return response;
}

Endpoint::Endpoint(std::string endpoint_id,
                   SessionRole role,
                   TransportConfig config)
    : endpoint_id_(std::move(endpoint_id))
    , session_(role, std::move(config)) {}

const std::string& Endpoint::endpoint_id() const {
    return endpoint_id_;
}

Session& Endpoint::session() {
    return session_;
}

const Session& Endpoint::session() const {
    return session_;
}

void Endpoint::connect(const std::string& peer_id) {
    session_.begin_handshake(peer_id);
    session_.activate();
}

std::uint64_t Endpoint::submit_request(Http3Request request) {
    if (!session_.is_ready()) {
        connect("peer");
    }
    if (request.stream_id == 0 && !request.body.empty()) {
        request.stream_id = session_.open_stream("request");
    } else if (request.stream_id == 0) {
        request.stream_id = session_.open_stream("request");
    }
    session_.send_request(request);
    return request.stream_id;
}

bool Endpoint::submit_response(Http3Response response) {
    if (!session_.is_ready()) {
        connect("peer");
    }
    return session_.send_response(response);
}

std::size_t Endpoint::flush_to(Endpoint& peer) {
    std::vector<OutboundDatagram> datagrams = session_.drain_outbound_datagrams();
    for (const auto& datagram : datagrams) {
        peer.session_.receive_datagram(datagram);
    }
    return datagrams.size();
}

std::size_t Endpoint::service(const RouteRegistry& routes) {
    std::size_t handled = 0;
    while (session_.has_pending_requests()) {
        Http3Request request = session_.pop_request();
        Http3Response response = routes.dispatch(request);
        response.stream_id = request.stream_id;
        session_.send_response(response);
        ++handled;
    }
    return handled;
}

bool Endpoint::has_response() const {
    return session_.has_pending_responses();
}

Http3Response Endpoint::pop_response() {
    return session_.pop_response();
}

LoopbackExchange::LoopbackExchange(TransportConfig config)
    : client_("client", SessionRole::Client, config)
    , server_("server", SessionRole::Server, std::move(config)) {
    client_.connect("server");
    server_.connect("client");
}

Endpoint& LoopbackExchange::client() {
    return client_;
}

Endpoint& LoopbackExchange::server() {
    return server_;
}

RouteRegistry& LoopbackExchange::routes() {
    return routes_;
}

const RouteRegistry& LoopbackExchange::routes() const {
    return routes_;
}

Http3Response LoopbackExchange::perform(Http3Request request) {
    const std::uint64_t stream_id = client_.submit_request(std::move(request));
    client_.flush_to(server_);
    server_.service(routes_);
    server_.flush_to(client_);

    if (!client_.has_response()) {
        throw std::runtime_error("Loopback exchange did not produce an HTTP/3 response");
    }

    Http3Response response = client_.pop_response();
    response.stream_id = stream_id;
    return response;
}

Http3Request request_from_http1(const Request& request, std::uint64_t stream_id) {
    Http3Request result;
    result.stream_id = stream_id;
    result.method = request.method.empty() ? "GET" : request.method;
    result.path = request.uri.empty() ? "/" : request.uri;
    result.body = request.body;
    for (const auto& [key, value] : request.headers) {
        HeaderField header;
        header.name = std::holds_alternative<HeaderKey>(key)
            ? header_key_to_string(std::get<HeaderKey>(key))
            : std::get<std::string>(key);
        header.value = value;
        result.headers.push_back(std::move(header));
    }
    return result;
}

Request request_to_http1(const Http3Request& request) {
    Request result;
    result.method = request.method;
    result.uri = request.path;
    result.body = request.body;
    for (const auto& header : request.headers) {
        const HeaderKey key = header_key_from_string(header.name);
        if (key == HeaderKey::Custom) {
            result.headers[header.name] = header.value;
        } else {
            result.headers[key] = header.value;
        }
    }
    return result;
}

Http3Response response_from_http1(const Response& response, std::uint64_t stream_id) {
    Http3Response result;
    result.stream_id = stream_id;
    result.status_code = response.status_code;
    result.body = response.body;
    for (const auto& [key, value] : response.headers) {
        HeaderField header;
        header.name = std::holds_alternative<HeaderKey>(key)
            ? header_key_to_string(std::get<HeaderKey>(key))
            : std::get<std::string>(key);
        header.value = value;
        result.headers.push_back(std::move(header));
    }
    return result;
}

Response response_to_http1(const Http3Response& response) {
    Response result;
    result.status_code = response.status_code;
    result.body = response.body;
    for (const auto& header : response.headers) {
        const HeaderKey key = header_key_from_string(header.name);
        if (key == HeaderKey::Custom) {
            result.headers[header.name] = header.value;
        } else {
            result.headers[key] = header.value;
        }
    }
    return result;
}

} // namespace quic
} // namespace networking