#pragma once
#include "http_servlet_base.h"

namespace networking {
namespace servlets {

class Http3Servlet : public HttpServletBase {
public:
    ~Http3Servlet() override = default;
    std::string get_version() const override { return "HTTP/3"; }
};

} // namespace servlets
} // namespace networking
