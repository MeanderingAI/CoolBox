
#ifndef NETWORKING_REST_API_SERVER_H
#define NETWORKING_REST_API_SERVER_H

#include "networking/http/request_response.h"
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>


namespace nhh = networking::http;
namespace networking {
namespace rest_api {

using Handler = std::function<nhh::Response(const nhh::Request&)>;

// Thread pool for handling requests
class ThreadPool {
public:
    ThreadPool(size_t num_threads = 4);
    ~ThreadPool();
    
    void enqueue(std::function<void()> task);
    void stop();
    size_t size() const { return threads_.size(); }
    
private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_;
    
    void worker();
};

// Route definition
class Route {
public:
    Route(const std::string& pattern, nhh::HttpMethod method, Handler handler);
    
    bool matches(const std::string& path, nhh::HttpMethod method) const;
    nhh::Response handle(const nhh::Request& request) const;
    std::map<std::string, std::string> extract_params(const std::string& path) const;

    // Getters for method and pattern
    nhh::HttpMethod method() const { return method_; }
    const std::string& pattern() const { return pattern_; }
    
private:
    std::string pattern_;
    nhh::HttpMethod method_;
    Handler handler_;
    std::vector<std::string> param_names_;
};

// REST API Server
class Server {
public:
    Server(int port, size_t num_threads = 4);
    ~Server();
    
    // Route registration
    void get(const std::string& pattern, Handler handler);
    void post(const std::string& pattern, Handler handler);
    void put(const std::string& pattern, Handler handler);
    void delete_(const std::string& pattern, Handler handler);
    void patch(const std::string& pattern, Handler handler);
    
    // Middleware
    void use_middleware(std::function<nhh::Response(const nhh::Request&, std::function<nhh::Response(const nhh::Request&)>)> middleware);
    
    // CORS support
    void enable_cors(const std::string& origin = "*");
    
    // Start/stop server
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Handle a single request (for testing)
    nhh::Response handle_request(const nhh::Request& request);
    
    // Handle request asynchronously using thread pool
    void handle_request_async(const nhh::Request& request, std::function<void(const nhh::Response&)> callback);
    
    int port() const { return port_; }
    size_t num_threads() const { return thread_pool_ ? thread_pool_->size() : 0; }
    
private:
    void add_route(const std::string& pattern, nhh::HttpMethod method, Handler handler);
    nhh::Response apply_middleware(const nhh::Request& request, Handler final_handler);
    
    int port_;
    bool running_;
    std::vector<std::shared_ptr<Route>> routes_;
    std::vector<std::function<nhh::Response(const nhh::Request&, std::function<nhh::Response(const nhh::Request&)>)>> middleware_;
    bool cors_enabled_;
    std::string cors_origin_;
    std::unique_ptr<ThreadPool> thread_pool_;
};

// (Now deprecated - use dataformats::json:: library directly)
namespace json_util {
    inline std::string encode(const std::map<std::string, std::string>& data) {
        return dataformats::json::simple::encode(data);
    }
    inline std::map<std::string, std::string> decode(const std::string& json_str) {
        return dataformats::json::simple::decode(json_str);
    }
    inline std::string encode_array(const std::vector<std::string>& data) {
        return dataformats::json::simple::encode_array(data);
    }
    inline std::vector<std::string> decode_array(const std::string& json_str) {
        return dataformats::json::simple::decode_array(json_str);
    }
}


} // namespace rest_api
} // namespace networking

#endif // NETWORKING_REST_API_SERVER_H
