#include "rest_api/server.h"
#include "json/json.h"
#include <sstream>
#include <regex>
#include <iostream>

namespace ml {
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

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            
            if (stop_ && tasks_.empty()) {
                return;
            }
            
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        
        if (task) {
            task();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stop_) {
            throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
        }
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    
    for (std::thread& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// =========================================================================
// Request Implementation
// =========================================================================

Request::Request(HttpMethod method, const std::string& path,
                 const std::map<std::string, std::string>& headers,
                 const std::string& body)
    : method_(method), path_(path), headers_(headers), body_(body) {
    parse_query_params();
}

std::string Request::get_header(const std::string& key, const std::string& default_val) const {
    auto it = headers_.find(key);
    return (it != headers_.end()) ? it->second : default_val;
}

std::string Request::get_query_param(const std::string& key, const std::string& default_val) const {
    auto it = query_params_.find(key);
    return (it != query_params_.end()) ? it->second : default_val;
}

std::string Request::get_path_param(const std::string& key, const std::string& default_val) const {
    auto it = path_params_.find(key);
    return (it != path_params_.end()) ? it->second : default_val;
}

void Request::set_path_params(const std::map<std::string, std::string>& params) {
    path_params_ = params;
}

void Request::parse_query_params() {
    size_t query_pos = path_.find('?');
    if (query_pos == std::string::npos) {
        return;
    }
    
    std::string query = path_.substr(query_pos + 1);
    std::istringstream iss(query);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            query_params_[key] = value;
        }
    }
}

// =========================================================================
// Response Implementation
// =========================================================================

Response::Response() : status_code_(200) {
    headers_["Content-Type"] = "text/plain";
}

Response::Response(int status_code, const std::string& body)
    : status_code_(status_code), body_(body) {
    headers_["Content-Type"] = "text/plain";
}

void Response::set_status(int code) {
    status_code_ = code;
}

void Response::set_status(HttpStatus status) {
    status_code_ = static_cast<int>(status);
}

void Response::set_body(const std::string& body) {
    body_ = body;
}

void Response::set_header(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void Response::set_json(const std::string& json) {
    body_ = json;
    headers_["Content-Type"] = "application/json";
}

std::string Response::to_string() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code_ << "\r\n";
    
    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }
    
    oss << "Content-Length: " << body_.length() << "\r\n";
    oss << "\r\n";
    oss << body_;
    
    return oss.str();
}

// =========================================================================
// Route Implementation
// =========================================================================

Route::Route(const std::string& pattern, HttpMethod method, Handler handler)
    : pattern_(pattern), method_(method), handler_(handler) {
    // Extract parameter names from pattern
    std::regex param_regex(":([a-zA-Z_][a-zA-Z0-9_]*)");
    std::smatch match;
    std::string temp = pattern_;
    
    while (std::regex_search(temp, match, param_regex)) {
        param_names_.push_back(match[1].str());
        temp = match.suffix();
    }
}

bool Route::matches(const std::string& path, HttpMethod method) const {
    if (method != method_) {
        return false;
    }
    
    // Remove query string
    std::string clean_path = path;
    size_t query_pos = clean_path.find('?');
    if (query_pos != std::string::npos) {
        clean_path = clean_path.substr(0, query_pos);
    }
    
    // Simple pattern matching (replace :param with regex)
    std::string regex_pattern = pattern_;
    regex_pattern = std::regex_replace(regex_pattern, std::regex(":([a-zA-Z_][a-zA-Z0-9_]*)"), "([^/]+)");
    regex_pattern = "^" + regex_pattern + "$";
    
    return std::regex_match(clean_path, std::regex(regex_pattern));
}

std::map<std::string, std::string> Route::extract_params(const std::string& path) const {
    std::map<std::string, std::string> params;
    
    // Remove query string
    std::string clean_path = path;
    size_t query_pos = clean_path.find('?');
    if (query_pos != std::string::npos) {
        clean_path = clean_path.substr(0, query_pos);
    }
    
    std::string regex_pattern = pattern_;
    regex_pattern = std::regex_replace(regex_pattern, std::regex(":([a-zA-Z_][a-zA-Z0-9_]*)"), "([^/]+)");
    regex_pattern = "^" + regex_pattern + "$";
    
    std::smatch match;
    if (std::regex_match(clean_path, match, std::regex(regex_pattern))) {
        for (size_t i = 0; i < param_names_.size() && i + 1 < match.size(); ++i) {
            params[param_names_[i]] = match[i + 1].str();
        }
    }
    
    return params;
}

Response Route::handle(const Request& request) const {
    return handler_(request);
}

// =========================================================================
// Server Implementation
// =========================================================================

Server::Server(int port, size_t num_threads)
    : port_(port), running_(false), cors_enabled_(false), cors_origin_("*"),
      thread_pool_(std::make_unique<ThreadPool>(num_threads)) {
}

Server::~Server() {
    stop();
}

void Server::get(const std::string& pattern, Handler handler) {
    add_route(pattern, HttpMethod::GET, handler);
}

void Server::post(const std::string& pattern, Handler handler) {
    add_route(pattern, HttpMethod::POST, handler);
}

void Server::put(const std::string& pattern, Handler handler) {
    add_route(pattern, HttpMethod::PUT, handler);
}

void Server::delete_(const std::string& pattern, Handler handler) {
    add_route(pattern, HttpMethod::DELETE, handler);
}

void Server::patch(const std::string& pattern, Handler handler) {
    add_route(pattern, HttpMethod::PATCH, handler);
}

void Server::add_route(const std::string& pattern, HttpMethod method, Handler handler) {
    routes_.push_back(std::make_shared<Route>(pattern, method, handler));
}

void Server::use_middleware(std::function<Response(const Request&, std::function<Response(const Request&)>)> middleware) {
    middleware_.push_back(middleware);
}

void Server::enable_cors(const std::string& origin) {
    cors_enabled_ = true;
    cors_origin_ = origin;
}

Response Server::apply_middleware(const Request& request, Handler final_handler) {
    if (middleware_.empty()) {
        return final_handler(request);
    }
    
    // Build middleware chain
    std::function<Response(const Request&)> handler = final_handler;
    for (auto it = middleware_.rbegin(); it != middleware_.rend(); ++it) {
        auto current_middleware = *it;
        auto next_handler = handler;
        handler = [current_middleware, next_handler](const Request& req) {
            return current_middleware(req, next_handler);
        };
    }
    
    return handler(request);
}

Response Server::handle_request(const Request& request) {
    // Find matching route
    for (auto& route : routes_) {
        if (route->matches(request.path(), request.method())) {
            // Extract path parameters
            auto params = route->extract_params(request.path());
            Request modified_request = request;
            modified_request.set_path_params(params);
            
            // Apply middleware and handle
            Response response = apply_middleware(modified_request, [&route](const Request& req) {
                return route->handle(req);
            });
            
            // Add CORS headers if enabled
            if (cors_enabled_) {
                response.set_header("Access-Control-Allow-Origin", cors_origin_);
                response.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
                response.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            }
            
            return response;
        }
    }
    
    // No route found
    Response response;
    response.set_status(HttpStatus::NOT_FOUND);
    response.set_json("{\"error\": \"Not Found\"}");
    return response;
}

void Server::handle_request_async(const Request& request, std::function<void(const Response&)> callback) {
    thread_pool_->enqueue([this, request, callback]() {
        Response response = handle_request(request);
        if (callback) {
            callback(response);
        }
    });
}

void Server::start() {
    running_ = true;
    std::cout << "Server started on port " << port_ << std::endl;
    std::cout << "Thread pool size: " << thread_pool_->size() << " threads" << std::endl;
    std::cout << "Note: This is a mock server for Python bindings." << std::endl;
    std::cout << "Use handle_request() to process requests programmatically." << std::endl;
}

void Server::stop() {
    running_ = false;
    if (thread_pool_) {
        thread_pool_->stop();
    }
    std::cout << "Server stopped" << std::endl;
}

} // namespace rest_api
} // namespace ml
