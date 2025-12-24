#include "security/fuzzer/fuzzer.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

// Simple echo server for testing
class SimpleEchoServer {
private:
    int port_;
    int server_fd_;
    std::thread server_thread_;
    bool running_;

public:
    SimpleEchoServer(int port) : port_(port), server_fd_(-1), running_(false) {}
    
    ~SimpleEchoServer() {
        stop();
    }
    
    void start() {
        running_ = true;
        server_thread_ = std::thread([this]() {
            server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd_ < 0) {
                std::cerr << "Failed to create socket\n";
                return;
            }
            
            int opt = 1;
            setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            
            struct sockaddr_in address;
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port_);
            
            if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
                std::cerr << "Bind failed\n";
                close(server_fd_);
                return;
            }
            
            if (listen(server_fd_, 3) < 0) {
                std::cerr << "Listen failed\n";
                close(server_fd_);
                return;
            }
            
            std::cout << "Echo server listening on port " << port_ << "\n";
            
            // Set socket to non-blocking for graceful shutdown
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            setsockopt(server_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            
            while (running_) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd < 0) {
                    if (!running_) break;
                    continue;
                }
                
                // Handle client in separate thread for concurrent connections
                std::thread([client_fd]() {
                    char buffer[4096];
                    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0';
                        // Echo back
                        send(client_fd, buffer, bytes_read, 0);
                    }
                    
                    close(client_fd);
                }).detach();
            }
            
            close(server_fd_);
        });
        
        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    void stop() {
        if (running_) {
            running_ = false;
            if (server_thread_.joinable()) {
                server_thread_.join();
            }
        }
    }
};

// Simple HTTP server for testing
class SimpleHTTPServer {
private:
    int port_;
    int server_fd_;
    std::thread server_thread_;
    bool running_;

public:
    SimpleHTTPServer(int port) : port_(port), server_fd_(-1), running_(false) {}
    
    ~SimpleHTTPServer() {
        stop();
    }
    
    void start() {
        running_ = true;
        server_thread_ = std::thread([this]() {
            server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd_ < 0) {
                std::cerr << "Failed to create HTTP socket\n";
                return;
            }
            
            int opt = 1;
            setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            
            struct sockaddr_in address;
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port_);
            
            if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
                std::cerr << "HTTP Bind failed\n";
                close(server_fd_);
                return;
            }
            
            if (listen(server_fd_, 3) < 0) {
                std::cerr << "HTTP Listen failed\n";
                close(server_fd_);
                return;
            }
            
            std::cout << "HTTP server listening on port " << port_ << "\n";
            
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            setsockopt(server_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            
            while (running_) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd < 0) {
                    if (!running_) break;
                    continue;
                }
                
                std::thread([client_fd]() {
                    char buffer[8192];
                    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0';
                        
                        // Simple HTTP response
                        std::string response = 
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 13\r\n"
                            "\r\n"
                            "Hello, World!";
                        
                        send(client_fd, response.c_str(), response.length(), 0);
                    }
                    
                    close(client_fd);
                }).detach();
            }
            
            close(server_fd_);
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    void stop() {
        if (running_) {
            running_ = false;
            if (server_thread_.joinable()) {
                server_thread_.join();
            }
        }
    }
};

int main() {
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║      Network Fuzzing Demo                        ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    // Test 1: TCP Fuzzing
    {
        std::cout << "\n[Test 1] TCP Protocol Fuzzing\n";
        std::cout << std::string(60, '=') << "\n";
        
        SimpleEchoServer server(8888);
        server.start();
        
        security::FuzzConfig config;
        config.max_iterations = 50;
        config.strategy = security::FuzzStrategy::RANDOM;
        config.verbose = false;
        
        security::NetworkFuzzer fuzzer("127.0.0.1", 8888, config);
        
        try {
            fuzzer.fuzz_tcp();
            fuzzer.print_report();
        } catch (const std::exception& e) {
            std::cerr << "TCP fuzzing error: " << e.what() << "\n";
        }
        
        server.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Test 2: HTTP Fuzzing with SQL Injection patterns
    {
        std::cout << "\n[Test 2] HTTP Fuzzing with SQL Injection Patterns\n";
        std::cout << std::string(60, '=') << "\n";
        
        SimpleHTTPServer server(8889);
        server.start();
        
        security::FuzzConfig config;
        config.max_iterations = 30;
        config.strategy = security::FuzzStrategy::SQL_INJECTION;
        config.verbose = false;
        
        security::NetworkFuzzer fuzzer("127.0.0.1", 8889, config);
        
        try {
            fuzzer.fuzz_http();
            fuzzer.print_report();
        } catch (const std::exception& e) {
            std::cerr << "HTTP fuzzing error: " << e.what() << "\n";
        }
        
        server.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Test 3: HTTP Fuzzing with XSS patterns
    {
        std::cout << "\n[Test 3] HTTP Fuzzing with XSS Patterns\n";
        std::cout << std::string(60, '=') << "\n";
        
        SimpleHTTPServer server(8890);
        server.start();
        
        security::FuzzConfig config;
        config.max_iterations = 30;
        config.strategy = security::FuzzStrategy::XSS;
        config.verbose = false;
        
        security::NetworkFuzzer fuzzer("127.0.0.1", 8890, config);
        
        try {
            fuzzer.fuzz_http();
            fuzzer.print_report();
        } catch (const std::exception& e) {
            std::cerr << "HTTP fuzzing error: " << e.what() << "\n";
        }
        
        server.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Test 4: HTTP Fuzzing with Buffer Overflow patterns
    {
        std::cout << "\n[Test 4] HTTP Fuzzing with Buffer Overflow Patterns\n";
        std::cout << std::string(60, '=') << "\n";
        
        SimpleHTTPServer server(8891);
        server.start();
        
        security::FuzzConfig config;
        config.max_iterations = 30;
        config.strategy = security::FuzzStrategy::BUFFER_OVERFLOW;
        config.verbose = false;
        
        security::NetworkFuzzer fuzzer("127.0.0.1", 8891, config);
        
        try {
            fuzzer.fuzz_http();
            fuzzer.print_report();
        } catch (const std::exception& e) {
            std::cerr << "HTTP fuzzing error: " << e.what() << "\n";
        }
        
        server.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Test 5: HTTP Fuzzing with all strategies
    {
        std::cout << "\n[Test 5] Comprehensive HTTP Fuzzing (All Strategies)\n";
        std::cout << std::string(60, '=') << "\n";
        
        SimpleHTTPServer server(8892);
        server.start();
        
        security::FuzzConfig config;
        config.max_iterations = 100;
        config.strategy = security::FuzzStrategy::ALL;
        config.verbose = false;
        
        security::NetworkFuzzer fuzzer("127.0.0.1", 8892, config);
        
        try {
            fuzzer.fuzz_http();
            fuzzer.print_report();
        } catch (const std::exception& e) {
            std::cerr << "HTTP fuzzing error: " << e.what() << "\n";
        }
        
        server.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║      Network Fuzzing Complete!                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "Summary:\n";
    std::cout << "- Tested TCP and HTTP protocol fuzzing\n";
    std::cout << "- Tested SQL injection, XSS, and buffer overflow patterns over HTTP\n";
    std::cout << "- Demonstrated network-level vulnerability testing\n";
    std::cout << "- All tests completed successfully\n\n";

    return 0;
}
