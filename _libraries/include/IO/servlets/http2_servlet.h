#pragma once
#include "networking/servlets/http_servlet_base.h"

namespace networking {
namespace servlets {

class Http2Servlet : public HttpServletBase {
public:
    ~Http2Servlet() override = default;
};

} // namespace servlets
} // namespace networking
