#pragma once
#include "networking/http/request_response.h"

namespace networking {
namespace servlets {

// Base class for all HTTP servlets (HTTP/1, HTTP/2, HTTP/3)
class HttpServletBase {
public:
    virtual ~HttpServletBase() = default;
    // All HTTP servlets must implement this
    virtual http::Response handle_request(const http::Request& request) = 0;
};

} // namespace servlets
} // namespace networking
