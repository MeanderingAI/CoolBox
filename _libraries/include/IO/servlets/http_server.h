#pragma once
#include "IO/servlets/HttpVersion.h"
#include "utils/thread_pool/thread_pool.h"
#include <string>
#include <memory>

namespace io {
namespace servlets {

class HttpServer {
public:
    HttpServer(HttpVersion version, size_t num_threads);
    ~HttpServer();

    void start();
    void stop();
    HttpVersion get_version() const;

private:
    HttpVersion version_;
    std::unique_ptr<ThreadPool> thread_pool_;
    // Add more server state as needed
};

} // namespace servlets
} // namespace io
