#ifndef ML_REST_API_SERVER_H
#define ML_REST_API_SERVER_H

#include "dataformats/json/json.h"
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace ml {
namespace rest_api {

// HTTP methods
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    OPTIONS
};

// HTTP status codes
enum class HttpStatus {
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

// Request object
class Request {
public:
    Request(HttpMethod method, const std::string& path,
            const std::map<std::string, std::string>& headers,
            const std::string& body);
    
    HttpMethod method() const { return method_; }
    std::string path() const { return path_; }
    std::string body() const { return body_; }
    std::map<std::string, std::string> headers() const { return headers_; }
    std::map<std::string, std::string> query_params() const { return query_params_; }
    std::map<std::string, std::string> path_params() const { return path_params_; }
    
    std::string get_header(const std::string& key, const std::string& default_val = "") const;
    std::string get_query_param(const std::string& key, const std::string& default_val = "") const;
    std::string get_path_param(const std::string& key, const std::string& default_val = "") const;
    
    void set_path_params(const std::map<std::string, std::string>& params);
    
private:
    HttpMethod method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    std::map<std::string, std::string> query_params_;
    std::map<std::string, std::string> path_params_;
    std::string body_;
    
    void parse_query_params();
};

// Response object
class Response {
public:
    Response();
    Response(int status_code, const std::string& body);
    
    int status_code() const { return status_code_; }
    std::string body() const { return body_; }
    std::map<std::string, std::string> headers() const { return headers_; }
    
    void set_status(int code);
    void set_status(HttpStatus status);
    void set_body(const std::string& body);
    void set_header(const std::string& key, const std::string& value);
    void set_json(const std::string& json);
    
    std::string to_string() const;
    
private:
    int status_code_;
    std::string body_;
    std::map<std::string, std::string> headers_;
};

// Handler function type
using Handler = std::function<Response(const Request&)>;

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
    Route(const std::string& pattern, HttpMethod method, Handler handler);
    
    bool matches(const std::string& path, HttpMethod method) const;
    Response handle(const Request& request) const;
    std::map<std::string, std::string> extract_params(const std::string& path) const;
    
private:
    std::string pattern_;
    HttpMethod method_;
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
    void use_middleware(std::function<Response(const Request&, std::function<Response(const Request&)>)> middleware);
    
    // CORS support
    void enable_cors(const std::string& origin = "*");
    
    // Start/stop server
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Handle a single request (for testing)
    Response handle_request(const Request& request);
    
    // Handle request asynchronously using thread pool
    void handle_request_async(const Request& request, std::function<void(const Response&)> callback);
    
    int port() const { return port_; }
    size_t num_threads() const { return thread_pool_ ? thread_pool_->size() : 0; }
    
private:
    void add_route(const std::string& pattern, HttpMethod method, Handler handler);
    Response apply_middleware(const Request& request, Handler final_handler);
    
    int port_;
    bool running_;
    std::vector<std::shared_ptr<Route>> routes_;
    std::vector<std::function<Response(const Request&, std::function<Response(const Request&)>)>> middleware_;
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
} // namespace ml

#endif // ML_REST_API_SERVER_H
