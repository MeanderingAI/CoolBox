#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "services/system_monitor/system_monitor.h"

class MetricsBackendService {
private:
    int port_;
    int server_fd_;
    services::SystemMonitor monitor_;
    bool running_;

    std::string generate_metrics_json() {
        auto metrics = monitor_.get_metrics();
        std::ostringstream json;
        json << "{\n"
             << "  \"cpu\": " << metrics.cpu_usage << ",\n"
             << "  \"memory\": " << metrics.memory_usage << ",\n"
             << "  \"disk\": " << metrics.disk_usage << ",\n"
             << "  \"network_rx\": " << metrics.network_rx_mbps << ",\n"
             << "  \"network_tx\": " << metrics.network_tx_mbps << ",\n"
             << "  \"timestamp\": " << time(nullptr) << "\n"
             << "}";
        return json.str();
    }

    std::string http_response(const std::string& body, const std::string& content_type = "application/json") {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: " << content_type << "\r\n"
                 << "Content-Length: " << body.length() << "\r\n"
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                 << "Access-Control-Allow-Headers: Content-Type\r\n"
                 << "Connection: close\r\n"
                 << "\r\n"
                 << body;
        return response.str();
    }

    void handle_request(int client_socket) {
        char buffer[4096] = {0};
        ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            close(client_socket);
            return;
        }

        std::string request(buffer);
        
        // Handle OPTIONS for CORS preflight
        if (request.find("OPTIONS") == 0) {
            std::string response = "HTTP/1.1 204 No Content\r\n"
                                 "Access-Control-Allow-Origin: *\r\n"
                                 "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                                 "Access-Control-Allow-Headers: Content-Type\r\n"
                                 "Connection: close\r\n\r\n";
            send(client_socket, response.c_str(), response.length(), 0);
            close(client_socket);
            return;
        }

        // Handle GET /metrics
        if (request.find("GET /metrics") != std::string::npos || 
            request.find("GET /api/metrics") != std::string::npos) {
            std::string json = generate_metrics_json();
            std::string response = http_response(json);
            send(client_socket, response.c_str(), response.length(), 0);
        } 
        // Handle GET /health
        else if (request.find("GET /health") != std::string::npos) {
            std::string json = "{\"status\": \"healthy\", \"service\": \"metrics-backend\"}";
            std::string response = http_response(json);
            send(client_socket, response.c_str(), response.length(), 0);
        }
        else {
            std::string body = "{\"error\": \"Not Found\"}";
            std::string response = "HTTP/1.1 404 Not Found\r\n"
                                 "Content-Type: application/json\r\n"
                                 "Content-Length: " + std::to_string(body.length()) + "\r\n"
                                 "Connection: close\r\n\r\n" + body;
            send(client_socket, response.c_str(), response.length(), 0);
        }

        close(client_socket);
    }

public:
    MetricsBackendService(int port) : port_(port), server_fd_(-1), running_(false) {}

    ~MetricsBackendService() {
        stop();
    }

    bool start() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "Failed to create socket\n";
            return false;
        }

        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options\n";
            close(server_fd_);
            return false;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Failed to bind to port " << port_ << "\n";
            close(server_fd_);
            return false;
        }

        if (listen(server_fd_, 10) < 0) {
            std::cerr << "Failed to listen on port " << port_ << "\n";
            close(server_fd_);
            return false;
        }

        running_ = true;
        std::cout << "\n=== Metrics Backend Service ===\n";
        std::cout << "✓ Started on port " << port_ << "\n";
        std::cout << "✓ Endpoints:\n";
        std::cout << "  - GET /metrics       - System metrics (JSON)\n";
        std::cout << "  - GET /api/metrics   - System metrics (JSON)\n";
        std::cout << "  - GET /health        - Health check\n";
        std::cout << "✓ CORS enabled for all origins\n";
        std::cout << "\nPress Ctrl+C to stop\n\n";

        return true;
    }

    void run() {
        if (!running_) {
            std::cerr << "Service not started\n";
            return;
        }

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        while (running_) {
            int client_socket = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (running_) {
                    std::cerr << "Failed to accept connection\n";
                }
                continue;
            }

            std::cout << "✓ Request from " << inet_ntoa(client_addr.sin_addr) << "\n";
            handle_request(client_socket);
        }
    }

    void stop() {
        running_ = false;
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
    }
};

int main(int argc, char* argv[]) {
    int port = 9001;  // Default port
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port < 1 || port > 65535) {
            std::cerr << "Invalid port number. Using default 9001\n";
            port = 9001;
        }
    }

    MetricsBackendService service(port);
    
    if (!service.start()) {
        return 1;
    }

    service.run();
    return 0;
}
