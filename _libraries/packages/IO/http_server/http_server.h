#ifndef IO_HTTP_SERVER_HTTP_SERVER_H
#define IO_HTTP_SERVER_HTTP_SERVER_H
#include "request_handle.h"
#include "HttpVersion.h"
#include "thread_pool.h"
#include "advanced_logging.h"
#include <string>
#include <memory>
#include <map>


namespace io {
namespace http_server {

class HttpServer {
public:
    // Display a banner with all available routes
    void display_banner() const;
    // Default error responses
    static std::string response_404() {
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
    }
    static std::string response_400() {
        return "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
    }
    static std::string response_500() {
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nInternal Server Error";
    }
public:
    HttpServer(int port, size_t num_threads, advanced_logging::Logger* logger);
    HttpServer(size_t num_threads); // keep for backward compatibility if needed
    ~HttpServer();

    void start();
    void stop();
    HttpVersion get_version() const;

    // Add a request handler
    void add_request_handler(const RequestHandle& handle);

    // Add multiple request handlers at once
    template<typename... Handles>
    void add_request_handler_group(const Handles&... handles) {
        (add_request_handler(handles), ...);
    }

private:
    HttpVersion version_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::map<std::string, RequestHandle> handlers_; // Key: method+path
    bool running_ = false;
    int port_ = 8080;
    advanced_logging::Logger* logger_ = nullptr;
    // Add more server state as needed
};

} // namespace http_server
} // namespace io

#endif // IO_HTTP_SERVER_HTTP_SERVER_H
