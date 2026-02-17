#ifndef IO_HTTP_SERVER_REQUEST_HANDLE_H
#define IO_HTTP_SERVER_REQUEST_HANDLE_H

#include <string>
#include <functional>
#include "../../dataformats/http/headers/request_response.h"

namespace io {
namespace http_server {


#include "HttpMethod.h"
#include <variant>

#if __cplusplus < 201703L
#error "RequestHandle with std::variant requires C++17 or newer."
#endif

// Struct describing a request handle
struct RequestHandle {
    using MethodType = std::variant<io::http_server::HttpMethod, std::string>;
    MethodType method;
    std::string path;
    std::function<Response(const std::string&)> handler; // Handler: takes request string, returns Response

    // Builder from lambda, method (enum or string), and path
    template<typename Lambda>
    static RequestHandle build(const Lambda& fn, MethodType method, const std::string& path) {
        return RequestHandle{method, path, fn};
    }

    // Helper: build from enum
    template<typename Lambda>
    static RequestHandle build(const Lambda& fn, io::http_server::HttpMethod method, const std::string& path) {
        return RequestHandle{method, path, fn};
    }

    // Helper: build from string
    template<typename Lambda>
    static RequestHandle build(const Lambda& fn, const std::string& method, const std::string& path) {
        return RequestHandle{method, path, fn};
    }

    // Get method as string
    std::string method_string() const {
        if (std::holds_alternative<io::http_server::HttpMethod>(method)) {
            return io::http_server::to_string(std::get<io::http_server::HttpMethod>(method));
        } else {
            return std::get<std::string>(method);
        }
    }
};

} // namespace http_server
} // namespace io

#endif // IO_HTTP_SERVER_REQUEST_HANDLE_H

