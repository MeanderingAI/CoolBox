
#include "http_server.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <unistd.h>
#endif


namespace io {
namespace http_server {


HttpServer::HttpServer(int port, size_t num_threads, advanced_logging::Logger* logger, std::shared_ptr<networking::servlets::HttpServletBase> servlet)
    : port_(port), num_threads_(num_threads), logger_(logger), servlet_(std::move(servlet)), thread_pool_(std::make_unique<ThreadPool>(num_threads)) {}


HttpServer::~HttpServer() {
    stop();
}


void HttpServer::start() {
    std::cout << "[HttpServer] Starting server on port " << port_ << ", version: " << get_version() << std::endl;
    running_ = true;

    // Initialize Winsock on Windows
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "[HttpServer] WSAStartup failed." << std::endl;
        return;
    }
#endif

#ifdef _WIN32
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "[HttpServer] Failed to create socket." << std::endl;
        WSACleanup();
        return;
    }
#else
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[HttpServer] Failed to create socket." << std::endl;
        return;
    }
#endif

    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), static_cast<int>(sizeof(opt)));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "[HttpServer] Failed to bind socket." << std::endl;
#ifdef _WIN32
            closesocket(server_fd);
            WSACleanup();
#else
            close(server_fd);
#endif
            return;
        }

        if (listen(server_fd, 10) < 0) {
            std::cerr << "[HttpServer] Failed to listen on socket." << std::endl;
#ifdef _WIN32
            closesocket(server_fd);
            WSACleanup();
#else
            close(server_fd);
#endif
            return;
        }

    std::cout << "[HttpServer] Listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in client_addr;
#ifdef _WIN32
        int client_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "[HttpServer] Accept failed." << std::endl;
            }
            continue;
        }

        thread_pool_->enqueue([this, client_fd]() {
#else
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "[HttpServer] Accept failed." << std::endl;
            }
            continue;
        }

        thread_pool_->enqueue([this, client_fd]() {
#endif
            char buffer[4096];
#ifdef _WIN32
            int bytes_read = recv(client_fd, buffer, static_cast<int>(sizeof(buffer) - 1), 0);
#else
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
#endif
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                Request req = Request::from_string(std::string(buffer));
                Response resp = servlet_->handle_request(req);
                std::string resp_str = resp.to_string();
#ifdef _WIN32
                send(client_fd, resp_str.c_str(), static_cast<int>(resp_str.size()), 0);
                closesocket(client_fd);
#else
                write(client_fd, resp_str.c_str(), resp_str.size());
                close(client_fd);
#endif
            }
        });
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
}



// Add stub for handler registration to resolve linker errors
void HttpServer::add_request_handler(const RequestHandle&) {}


void HttpServer::stop() {
    running_ = false;
}


void HttpServer::display_banner() const {
    std::cout << "[HttpServer] Banner: Service running on port " << port_ << ", version: " << get_version() << std::endl;
}


std::string HttpServer::get_version() const {
    return servlet_ ? servlet_->get_version() : "UNKNOWN";
}


} // namespace http_server
} // namespace io
