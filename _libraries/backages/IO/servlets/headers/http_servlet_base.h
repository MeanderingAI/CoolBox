#ifndef COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP_SERVLET_BASE_H
#define COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP_SERVLET_BASE_H


#include "../../dataformats/http/headers/request_response.h"

namespace networking {
namespace servlets {


// Abstract handler interface
class RequestHandler {
public:
    virtual ~RequestHandler() = default;
    virtual Response handle(const Request& request) = 0;
};

// Base class for all HTTP servlets (HTTP/1, HTTP/2, HTTP/3)
class HttpServletBase {
public:
    virtual ~HttpServletBase() = default;
    virtual Response handle_request(const Request& request) = 0;
    virtual std::string get_version() const = 0;
};

} // namespace servlets
} // namespace networking

#endif  // COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP_SERVLET_BASE_H
