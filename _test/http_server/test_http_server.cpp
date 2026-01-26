#include "gtest/gtest.h"
#include "IO/http_server/headers/http_server.h"
#include "IO/http_server/headers/HttpVersion.h"

using namespace io::http_server;

TEST(HttpServerTest, VersionIsSetCorrectly) {
    HttpServer server(HttpVersion::HTTP_2, 4);
    EXPECT_EQ(server.get_version(), HttpVersion::HTTP_2);
}

TEST(HttpServerTest, ThreadPoolCreated) {
    HttpServer server(HttpVersion::HTTP_1, 2);
    EXPECT_EQ(server.get_version(), HttpVersion::HTTP_1);
    // Additional checks for thread pool can be added if interface allows
}

// Add more tests for start/stop logic as implemented
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

TEST(HttpServerTest, CanSendAndReceiveResponse) {
    using namespace io::http_server;
    HttpServer server(HttpVersion::HTTP_1, 2);
    // Register a handler for GET /
    RequestHandle handle;
    handle.method = "GET";
    handle.path = "/";
    handle.handler = [](const std::string& req) {
        std::string body = "Hello, World!";
        return "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    };
    server.add_request_handler(handle);
    // Start server in a background thread
    std::thread server_thread([&server]() { server.start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Give server time to start
    // Connect to server and send request
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(sock, -1);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ASSERT_EQ(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), 0);
    std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(sock, request.c_str(), request.size(), 0);
    char buffer[4096] = {0};
    ssize_t valread = read(sock, buffer, sizeof(buffer) - 1);
    ASSERT_GT(valread, 0);
    std::string response(buffer, valread);
    EXPECT_NE(response.find("Hello, World!"), std::string::npos);
    close(sock);
    // Stop server
    server.stop();
    server_thread.join();
}
