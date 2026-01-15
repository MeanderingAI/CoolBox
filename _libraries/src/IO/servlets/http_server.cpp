#include "IO/servlets/http_server.h"

namespace io {
namespace servlets {

HttpServer::HttpServer(HttpVersion version, size_t num_threads)
    : version_(version), thread_pool_(std::make_unique<ThreadPool>(num_threads)) {}

HttpServer::~HttpServer() = default;

void HttpServer::start() {
    // Start server logic, e.g., listen for requests and dispatch to thread pool
}

void HttpServer::stop() {
    // Stop server logic
}

HttpVersion HttpServer::get_version() const {
    return version_;
}

} // namespace servlets
} // namespace io
