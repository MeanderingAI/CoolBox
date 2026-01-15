#pragma once
#include "networking/servlets/http_servlet_base.h"

namespace networking {
namespace servlets {

class Http3Servlet : public HttpServletBase {
public:
    ~Http3Servlet() override = default;
};

} // namespace servlets
} // namespace networking
