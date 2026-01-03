#ifndef NETWORKING_HTTP_REQUEST_RESPONSE_H
#define NETWORKING_HTTP_REQUEST_RESPONSE_H

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "dataformats/json/json.h"

namespace networking {
namespace http {

// HTTP methods
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE_,
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

} // namespace http
} // namespace networking

#endif // NETWORKING_HTTP_REQUEST_RESPONSE_H
