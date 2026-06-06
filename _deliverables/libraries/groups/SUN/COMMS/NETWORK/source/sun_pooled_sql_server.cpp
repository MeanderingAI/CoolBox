#include "sun_pooled_sql_server.hpp"

#include "thread_pool.h"

#include <cerrno>
#include <cstring>
#include <memory>
#include <string>

#if defined(_WIN32)
#error "sun_network currently supports POSIX sockets (macOS/Linux)."
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace sun::comms::network {
namespace {

constexpr int k_backlog = 16;
constexpr const char* k_end_marker = "<<END>>";

bool send_all(int fd, const std::string& payload) {
    std::size_t sent = 0;
    while (sent < payload.size()) {
        const ssize_t n = ::send(fd, payload.data() + sent, payload.size() - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

bool recv_line(int fd, std::string& out_line) {
    out_line.clear();
    char ch = '\0';
    while (true) {
        const ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n == 0) {
            return false;
        }
        if (n < 0) {
            return false;
        }
        if (ch == '\n') {
            break;
        }
        if (ch != '\r') {
            out_line.push_back(ch);
        }
    }
    return true;
}

std::string wire_message(const QueryResponse& response) {
    return std::string(response.ok ? "OK\n" : "ERR\n") + response.payload + "\n" + k_end_marker + "\n";
}

}  // namespace

PooledSqlServer::PooledSqlServer(ServerConfig config, QueryHandler handler)
    : config_(std::move(config)), handler_(std::move(handler)) {}

PooledSqlServer::~PooledSqlServer() {
    stop();
}

bool PooledSqlServer::run() {
    if (running_.exchange(true)) {
        return false;
    }

    auto pool = std::make_shared<ThreadPool>(config_.worker_threads > 0 ? config_.worker_threads : 1);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(config_.port));

    if (::inet_pton(AF_INET, config_.host.c_str(), &addr.sin_addr) != 1) {
        running_ = false;
        return false;
    }

    if (config_.enable_tcp) {
        tcp_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_fd_ < 0) {
            running_ = false;
            return false;
        }

        int reuse = 1;
        ::setsockopt(tcp_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        if (::bind(tcp_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(tcp_fd_);
            tcp_fd_ = -1;
            running_ = false;
            return false;
        }

        if (::listen(tcp_fd_, k_backlog) < 0) {
            ::close(tcp_fd_);
            tcp_fd_ = -1;
            running_ = false;
            return false;
        }

        tcp_accept_thread_ = std::thread([this, pool] {
            while (running_) {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                const int client_fd = ::accept(tcp_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                if (client_fd < 0) {
                    if (running_) {
                        continue;
                    }
                    break;
                }

                pool->enqueue([this, client_fd] {
                    const std::string banner = std::string("OK\nconnected\n") + k_end_marker + "\n";
                    if (!send_all(client_fd, banner)) {
                        ::close(client_fd);
                        return;
                    }

                    std::string query;
                    while (recv_line(client_fd, query)) {
                        if (query.empty()) {
                            continue;
                        }

                        if (query == ".exit" || query == "exit" || query == "quit") {
                            const QueryResponse bye{true, "bye"};
                            send_all(client_fd, wire_message(bye));
                            break;
                        }

                        const QueryResponse response = handler_(query);
                        if (!send_all(client_fd, wire_message(response))) {
                            break;
                        }
                    }

                    ::close(client_fd);
                });
            }
        });
    }

    if (config_.enable_udp) {
        udp_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_fd_ >= 0) {
            int reuse = 1;
            ::setsockopt(udp_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

            if (::bind(udp_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
                udp_loop_thread_ = std::thread([this, pool] {
                    while (running_) {
                        sockaddr_in client_addr{};
                        socklen_t client_len = sizeof(client_addr);
                        char buffer[4096];
                        const ssize_t n = ::recvfrom(udp_fd_, buffer, sizeof(buffer) - 1, 0,
                                                     reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                        if (n <= 0) {
                            if (!running_) {
                                break;
                            }
                            continue;
                        }

                        buffer[n] = '\0';
                        const std::string query(buffer);
                        pool->enqueue([this, query, client_addr, client_len] {
                            const QueryResponse response = handler_(query);
                            const std::string payload = wire_message(response);
                            ::sendto(udp_fd_, payload.data(), payload.size(), 0,
                                     reinterpret_cast<const sockaddr*>(&client_addr), client_len);
                        });
                    }
                });
            } else {
                ::close(udp_fd_);
                udp_fd_ = -1;
            }
        }
    }

    if (tcp_accept_thread_.joinable()) {
        tcp_accept_thread_.join();
    }
    if (udp_loop_thread_.joinable()) {
        udp_loop_thread_.join();
    }

    return true;
}

void PooledSqlServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (tcp_fd_ >= 0) {
        ::shutdown(tcp_fd_, SHUT_RDWR);
        ::close(tcp_fd_);
        tcp_fd_ = -1;
    }

    if (udp_fd_ >= 0) {
        ::close(udp_fd_);
        udp_fd_ = -1;
    }

    if (tcp_accept_thread_.joinable()) {
        tcp_accept_thread_.join();
    }
    if (udp_loop_thread_.joinable()) {
        udp_loop_thread_.join();
    }
}

void PooledSqlServer::tcp_accept_loop() {}
void PooledSqlServer::udp_loop() {}

}  // namespace sun::comms::network
