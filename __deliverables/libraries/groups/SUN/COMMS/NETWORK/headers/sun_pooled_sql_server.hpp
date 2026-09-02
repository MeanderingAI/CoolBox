#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace sun::comms::network {

struct ServerConfig {
    std::string host = "127.0.0.1";
    int port = 55432;
    std::size_t worker_threads = 4;
    bool enable_tcp = true;
    bool enable_udp = true;
};

struct QueryResponse {
    bool ok = true;
    std::string payload;
};

using QueryHandler = std::function<QueryResponse(const std::string&)>;

class PooledSqlServer {
public:
    explicit PooledSqlServer(ServerConfig config, QueryHandler handler);
    ~PooledSqlServer();

    bool run();
    void stop();

private:
    ServerConfig config_;
    QueryHandler handler_;
    std::atomic<bool> running_{false};

    int tcp_fd_ = -1;
    int udp_fd_ = -1;

    std::thread tcp_accept_thread_;
    std::thread udp_loop_thread_;

    void tcp_accept_loop();
    void udp_loop();
};

}  // namespace sun::comms::network
