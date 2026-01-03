#ifndef ML_DATAFORMATS_HTTP_HTTP_H
#define ML_DATAFORMATS_HTTP_HTTP_H

#include <string>
#include <map>
#include <memory>
#include "dataformats/json/json.h"

namespace dataformats {
namespace http {



class Request {
public:
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    dataformats::json::Value json_body;

    Request() = default;
    Request(const std::string& m, const std::string& p,
            const std::map<std::string, std::string>& h,
            const std::string& b)
        : method(m), path(p), headers(h), body(b) {}

};

class RequestBuilder {
private:
    Request req_;
public:
    RequestBuilder& method(const std::string& m) { req_.method = m; return *this; }
    RequestBuilder& path(const std::string& p) { req_.path = p; return *this; }
    RequestBuilder& header(const std::string& k, const std::string& v) { req_.headers[k] = v; return *this; }
    RequestBuilder& body(const std::string& b) { req_.body = b; return *this; }
    RequestBuilder& json(const dataformats::json::Value& j) { req_.json_body = j; return *this; }
    Request build() const { return req_; }
};
};

// ...existing code...

class Response {
public:
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    dataformats::json::Value json_body;

    Response() = default;
    Response(int code, const std::string& b)
        : status_code(code), body(b) {}
};


#endif // ML_DATAFORMATS_HTTP_HTTP_H
