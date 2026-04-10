#ifndef COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP2_SERVLET_H
#define COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP2_SERVLET_H
#include "http_servlet_base.h"

namespace networking {
namespace servlets {

class Http2Servlet : public HttpServletBase {
public:
    ~Http2Servlet() override = default;
    std::string get_version() const override { return "HTTP/2"; }
};

} // namespace servlets
} // namespace networking

#endif  // COOLBOX__LIBRARIES_BACKAGES_IO_SERVLETS_HEADERS_HTTP2_SERVLET_H
