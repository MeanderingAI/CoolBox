#pragma once
#include "networking/servlets/http_servlet_base.h"

namespace networking {
namespace servlets {

class Http1Servlet : public HttpServletBase {
public:
    ~Http1Servlet() override = default;
};

} // namespace servlets
} // namespace networking
