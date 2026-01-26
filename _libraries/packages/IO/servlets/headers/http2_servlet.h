#pragma once
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
