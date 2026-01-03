


// ...existing code...
#include "networking/rest_api/server.h"
#include "dataformats/json/json.h"
#include <sstream>
#include <regex>
#include <iostream>

namespace networking {
namespace rest_api {

// =========================================================================
// ThreadPool Implementation
// =========================================================================

ThreadPool::ThreadPool(size_t num_threads) : stop_(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        threads_.emplace_back(&ThreadPool::worker, this);
    }
}


ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

// Route Implementation
Route::Route(const std::string& pattern, nhh::HttpMethod method, Handler handler)
    : pattern_(pattern), method_(method), handler_(handler) {}

bool Route::matches(const std::string& path, nhh::HttpMethod method) const {
    // Simple match: exact path and method
    return (pattern_ == path && method_ == method);
}

nhh::Response Route::handle(const nhh::Request& request) const {
    return handler_(request);
}

std::map<std::string, std::string> Route::extract_params(const std::string& path) const {
    // Stub: no params extracted
    return {};
}

// Server Implementation
Server::Server(int port, size_t num_threads)
    : port_(port), running_(false), cors_enabled_(false), thread_pool_(std::make_unique<ThreadPool>(num_threads)) {}

Server::~Server() { stop(); }

void Server::get(const std::string& pattern, Handler handler) { add_route(pattern, nhh::HttpMethod::GET, handler); }
void Server::post(const std::string& pattern, Handler handler) { add_route(pattern, nhh::HttpMethod::POST, handler); }
void Server::put(const std::string& pattern, Handler handler) { add_route(pattern, nhh::HttpMethod::PUT, handler); }
void Server::delete_(const std::string& pattern, Handler handler) { add_route(pattern, nhh::HttpMethod::DELETE_, handler); }
void Server::patch(const std::string& pattern, Handler handler) { add_route(pattern, nhh::HttpMethod::PATCH, handler); }

void Server::start() { running_ = true; }
void Server::stop() { running_ = false; }




void Server::add_route(const std::string& pattern, nhh::HttpMethod method, Handler handler) {
    routes_.push_back(std::make_shared<Route>(pattern, method, handler));
}

} // namespace rest_api
} // namespace networking
