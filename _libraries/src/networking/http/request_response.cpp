#include "networking/http/request_response.h"
#include <sstream>

namespace networking {
namespace http {

Request::Request(HttpMethod method, const std::string& path,
                 const std::map<std::string, std::string>& headers,
                 const std::string& body)
    : method_(method), path_(path), headers_(headers), body_(body) {
    parse_query_params();
}

std::string Request::get_header(const std::string& key, const std::string& default_val) const {
    auto it = headers_.find(key);
    return it != headers_.end() ? it->second : default_val;
}

std::string Request::get_query_param(const std::string& key, const std::string& default_val) const {
    auto it = query_params_.find(key);
    return it != query_params_.end() ? it->second : default_val;
}

std::string Request::get_path_param(const std::string& key, const std::string& default_val) const {
    auto it = path_params_.find(key);
    return it != path_params_.end() ? it->second : default_val;
}

void Request::set_path_params(const std::map<std::string, std::string>& params) {
    path_params_ = params;
}

void Request::parse_query_params() {
    // Simple query param parser (assumes ?key=value&key2=value2)
    auto pos = path_.find('?');
    if (pos == std::string::npos) return;
    std::string query = path_.substr(pos + 1);
    std::istringstream iss(query);
    std::string token;
    while (std::getline(iss, token, '&')) {
        auto eq = token.find('=');
        if (eq != std::string::npos) {
            query_params_[token.substr(0, eq)] = token.substr(eq + 1);
        }
    }
}

Response::Response() : status_code_(200) {}

Response::Response(int status_code, const std::string& body)
    : status_code_(status_code), body_(body) {}

void Response::set_status(int code) { status_code_ = code; }

void Response::set_status(HttpStatus status) { status_code_ = static_cast<int>(status); }

void Response::set_body(const std::string& body) { body_ = body; }

void Response::set_header(const std::string& key, const std::string& value) { headers_[key] = value; }

void Response::set_json(const std::string& json) { body_ = json; headers_["Content-Type"] = "application/json"; }

std::string Response::to_string() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code_ << "\r\n";
    for (const auto& h : headers_) {
        oss << h.first << ": " << h.second << "\r\n";
    }
    oss << "\r\n" << body_;
    return oss.str();
}

} // namespace http
} // namespace networking
