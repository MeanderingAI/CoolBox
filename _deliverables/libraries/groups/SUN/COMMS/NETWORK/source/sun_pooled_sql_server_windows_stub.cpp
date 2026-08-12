#include "sun_pooled_sql_server.hpp"

#include <utility>

namespace sun::comms::network {

PooledSqlServer::PooledSqlServer(ServerConfig config, QueryHandler handler)
    : config_(std::move(config)), handler_(std::move(handler)) {}

PooledSqlServer::~PooledSqlServer() {
    stop();
}

bool PooledSqlServer::run() {
    // Windows stub: networking backend is POSIX-only in this implementation.
    running_ = false;
    return false;
}

void PooledSqlServer::stop() {
    running_ = false;
}

void PooledSqlServer::tcp_accept_loop() {}
void PooledSqlServer::udp_loop() {}

}  // namespace sun::comms::network
