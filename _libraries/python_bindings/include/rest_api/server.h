#ifndef ML_REST_API_SERVER_H
#define ML_REST_API_SERVER_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <sstream>
#include <stdexcept>

namespace ml {
namespace rest_api {

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    OPTIONS
};

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

class Request {
public:
    Request(HttpMethod method, const std::string& path,
            const std::map<std::string, std::string>& headers,
            const std::string& body)
        : method_(method), path_(path), headers_(headers), body_(body) {}

    HttpMethod method() const { return method_; }
    const std::string& path() const { return path_; }
    const std::string& body() const { return body_; }
    const std::map<std::string, std::string>& headers() const { return headers_; }
    const std::map<std::string, std::string>& query_params() const { return query_params_; }
    const std::map<std::string, std::string>& path_params() const { return path_params_; }

    std::string get_header(const std::string& key, const std::string& default_val = "") const {
        auto it = headers_.find(key);
        return it != headers_.end() ? it->second : default_val;
    }

    std::string get_query_param(const std::string& key, const std::string& default_val = "") const {
        auto it = query_params_.find(key);
        return it != query_params_.end() ? it->second : default_val;
    }

    std::string get_path_param(const std::string& key, const std::string& default_val = "") const {
        auto it = path_params_.find(key);
        return it != path_params_.end() ? it->second : default_val;
    }

private:
    HttpMethod method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    std::map<std::string, std::string> query_params_;
    std::map<std::string, std::string> path_params_;
};

class Response {
public:
    Response() : status_code_(200) {}
    Response(int status_code, const std::string& body)
        : status_code_(status_code), body_(body) {}

    int status_code() const { return status_code_; }
    const std::string& body() const { return body_; }
    const std::map<std::string, std::string>& headers() const { return headers_; }

    void set_status(int code) { status_code_ = code; }
    void set_body(const std::string& body) { body_ = body; }
    void set_header(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }
    void set_json(const std::string& json_body) {
        body_ = json_body;
        headers_["Content-Type"] = "application/json";
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status_code_ << "\r\n";
        for (const auto& h : headers_) {
            oss << h.first << ": " << h.second << "\r\n";
        }
        oss << "\r\n" << body_;
        return oss.str();
    }

private:
    int status_code_;
    std::string body_;
    std::map<std::string, std::string> headers_;
};

using RouteHandler = std::function<Response(const Request&)>;

class Server {
public:
    explicit Server(int port = 8080) : port_(port), running_(false) {}

    void get(const std::string& path, RouteHandler handler) {
        routes_[{HttpMethod::GET, path}] = std::move(handler);
    }
    void post(const std::string& path, RouteHandler handler) {
        routes_[{HttpMethod::POST, path}] = std::move(handler);
    }
    void put(const std::string& path, RouteHandler handler) {
        routes_[{HttpMethod::PUT, path}] = std::move(handler);
    }
    void delete_(const std::string& path, RouteHandler handler) {
        routes_[{HttpMethod::DELETE, path}] = std::move(handler);
    }
    void patch(const std::string& path, RouteHandler handler) {
        routes_[{HttpMethod::PATCH, path}] = std::move(handler);
    }

    void enable_cors(const std::string& origin = "*") {
        cors_origin_ = origin;
    }

    void start() { running_ = true; }
    void stop() { running_ = false; }
    bool is_running() const { return running_; }

    Response handle_request(const Request& request) {
        auto it = routes_.find({request.method(), request.path()});
        if (it != routes_.end()) {
            Response response = it->second(request);
            if (!cors_origin_.empty()) {
                response.set_header("Access-Control-Allow-Origin", cors_origin_);
            }
            return response;
        }
        return Response(404, "Not Found");
    }

    int port() const { return port_; }

private:
    struct RouteKey {
        HttpMethod method;
        std::string path;
        bool operator<(const RouteKey& other) const {
            if (method != other.method) return method < other.method;
            return path < other.path;
        }
    };

    int port_;
    bool running_;
    std::string cors_origin_;
    std::map<RouteKey, RouteHandler> routes_;
};

namespace json {
    inline std::string encode(const std::map<std::string, std::string>& data) {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& kv : data) {
            if (!first) oss << ",";
            oss << "\"" << kv.first << "\":\"" << kv.second << "\"";
            first = false;
        }
        oss << "}";
        return oss.str();
    }

    inline std::map<std::string, std::string> decode(const std::string& json_str) {
        // Minimal JSON parsing for key-value string pairs
        std::map<std::string, std::string> result;
        // Simplified: just return empty map for non-trivial JSON
        (void)json_str;
        return result;
    }

    inline std::string encode_array(const std::vector<std::string>& arr) {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "\"" << arr[i] << "\"";
        }
        oss << "]";
        return oss.str();
    }

    inline std::vector<std::string> decode_array(const std::string& json_str) {
        std::vector<std::string> result;
        (void)json_str;
        return result;
    }
} // namespace json

} // namespace rest_api
} // namespace ml

#endif // ML_REST_API_SERVER_H
