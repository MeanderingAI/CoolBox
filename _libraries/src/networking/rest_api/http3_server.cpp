#include "advanced_logging/advanced_logging.h"
static advanced_logging::Logger http3_logger("http3_server.log");
#include "networking/rest_api/http3_server.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>


namespace networking {
namespace rest_api {


Http3Server::Http3Server(int port, size_t num_threads)
	: HttpServerBase(port, num_threads, HttpVersion::HTTP_3) {
	settings_.max_idle_timeout = 30000;
	settings_.max_udp_payload_size = 1200;
	settings_.enable_0rtt = false;
}

Http3Server::~Http3Server() {
	stop();
}

ProtocolCapabilities Http3Server::capabilities() const {
	return ProtocolCapabilities::for_version(HttpVersion::HTTP_3);
}

void Http3Server::start() {
	running_ = true;
	std::cout << "HTTP/3 Server (quiche) starting on UDP port " << port_ << std::endl;

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return;
	}
	sockaddr_in servaddr{};
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port_);
	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind");
		close(sockfd);
		return;
	}

	std::cout << "UDP socket bound. Waiting for QUIC packets..." << std::endl;

	while (running_) {
		uint8_t buf[65535];
		sockaddr_in cliaddr{};
		socklen_t len = sizeof(cliaddr);
		ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, &len);
		if (n < 0) {
			if (errno == EINTR) continue;
			perror("recvfrom");
			break;
		}
		std::cout << "Received UDP datagram: " << n << " bytes" << std::endl;
	}

	close(sockfd);
	std::cout << "HTTP/3 Server stopped." << std::endl;
}

void Http3Server::stop() {
	running_ = false;
	if (thread_pool_) {
		thread_pool_->stop();
	}
	std::lock_guard<std::mutex> lock(connections_mutex_);
	connections_.clear();
	std::cout << "HTTP/3 Server stopped" << std::endl;
}

nhh::Response Http3Server::handle_request(const nhh::Request& request) {
	std::string conn_id = generate_connection_id();
	auto stream = create_stream(conn_id);
	nhh::Response response = process_routes(request);
	response.set_header("Server", "ToolBox/1.0 (HTTP/3)");
	response.set_header("Alt-Svc", "h3=\":8080\"; ma=86400");
	close_stream(conn_id, stream->id);
	return response;
}

void Http3Server::handle_request_async(const nhh::Request& request,
									   std::function<void(const nhh::Response&)> callback) {
	thread_pool_->enqueue([this, request, callback]() {
		nhh::Response response = handle_request(request);
		if (callback) {
			callback(response);
		}
	});
}

void Http3Server::enable_0rtt(bool enabled) {
	settings_.enable_0rtt = enabled;
}

void Http3Server::set_max_idle_timeout(uint64_t ms) {
	settings_.max_idle_timeout = ms;
}

void Http3Server::set_max_udp_payload_size(uint64_t size) {
	settings_.max_udp_payload_size = size;
}

std::string Http3Server::create_connection(const std::string& client_id) {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	std::string conn_id = generate_connection_id();
	auto& conn = connections_[conn_id];
	conn.connection_id = conn_id;
	conn.is_established = false;
	conn.zero_rtt_enabled = settings_.enable_0rtt;
	conn.initial_secret = generate_initial_secret();
	conn.packet_number = 0;
	conn.established_time = std::chrono::steady_clock::now();
	conn.last_activity = std::chrono::steady_clock::now();
	return conn_id;
}

void Http3Server::close_connection(const std::string& conn_id, uint64_t error_code) {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	auto it = connections_.find(conn_id);
	if (it != connections_.end()) {
		connections_.erase(it);
	}
}

bool Http3Server::is_connection_established(const std::string& conn_id) const {
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(connections_mutex_));
	auto it = connections_.find(conn_id);
	if (it != connections_.end()) {
		return it->second.is_established;
	}
	return false;
}

std::shared_ptr<Http2Stream> Http3Server::create_stream(const std::string& conn_id) {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	auto& conn = connections_[conn_id];
	uint64_t stream_id = conn.next_stream_id;
	conn.next_stream_id += 4;
	auto stream = std::make_shared<Http2Stream>(stream_id);
	stream->state = Http2StreamState::OPEN;
	conn.streams[stream_id] = stream;
	conn.last_activity = std::chrono::steady_clock::now();
	return stream;
}

void Http3Server::close_stream(const std::string& conn_id, uint64_t stream_id) {
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

std::string Http3Server::generate_connection_id() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dis(0, 0xFFFFFF);
	std::ostringstream oss;
	oss << std::hex << std::setfill('0') << std::setw(6) << dis(gen);
	return "h3_" + oss.str();
}

std::vector<uint8_t> Http3Server::generate_initial_secret() {
	std::vector<uint8_t> secret(32);
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dis(0, 255);
	for (auto& byte : secret) {
		byte = static_cast<uint8_t>(dis(gen));
	}
	return secret;
}

Http3Server::QuicPacket Http3Server::create_packet(const std::string& conn_id,
													const std::vector<uint8_t>& data) {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	auto& conn = connections_[conn_id];
	QuicPacket packet;
	packet.packet_number = conn.packet_number++;
	packet.payload = data;
	packet.is_initial = !conn.is_established;
	packet.is_0rtt = conn.zero_rtt_enabled && !conn.is_established;
	return packet;
}

void Http3Server::handle_packet(const std::string& conn_id, const QuicPacket& packet) {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	auto it = connections_.find(conn_id);
	if (it == connections_.end()) {
		return;
	}
	auto& conn = it->second;
	if (packet.is_initial) {
		conn.is_established = true;
	}
	conn.last_activity = std::chrono::steady_clock::now();
}

void Http3Server::handle_stream_frame(const std::string& conn_id, uint64_t stream_id,
									  const std::vector<uint8_t>& data) {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	auto conn_it = connections_.find(conn_id);
	if (conn_it != connections_.end()) {
		auto stream_it = conn_it->second.streams.find(stream_id);
		if (stream_it != conn_it->second.streams.end()) {
			stream_it->second->data = data;
		}
	}
}

void Http3Server::handle_connection_close(const std::string& conn_id, uint64_t error_code) {
	close_connection(conn_id, error_code);
}

std::vector<uint8_t> Http3Server::encode_headers(const std::string& conn_id,
												 const std::map<std::string, std::string>& headers) {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	auto& conn = connections_[conn_id];
	return conn.qpack_encoder.encode(headers);
}

std::map<std::string, std::string> Http3Server::decode_headers(const std::string& conn_id,
															   const std::vector<uint8_t>& data) {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	auto& conn = connections_[conn_id];
	return conn.qpack_encoder.decode(data);
}

void Http3Server::cleanup_idle_connections() {
	std::lock_guard<std::mutex> lock(connections_mutex_);
	auto now = std::chrono::steady_clock::now();
	auto it = connections_.begin();
	while (it != connections_.end()) {
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			now - it->second.last_activity).count();
		if (elapsed > static_cast<long>(settings_.max_idle_timeout)) {
			it = connections_.erase(it);
		} else {
			++it;
		}
	}
}

} // namespace rest_api
} // namespace networking


