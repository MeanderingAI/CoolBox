
#pragma once
#include "http_servlet_base.h"
#include "../../../../_binaries/apps/app_service_manager/include/static_assets/service_manager_html.h"
#include <fstream>
#include <sstream>

namespace networking {
namespace servlets {

class Http1Servlet : public HttpServletBase {
public:
    explicit Http1Servlet(std::shared_ptr<RequestHandler> handler) : handler_(std::move(handler)) {}
    ~Http1Servlet() override = default;
    std::string get_version() const override { return "HTTP/1.1"; }
    Response handle_request(const Request& request) override {
        return handler_->handle(request);
    }
private:
    std::shared_ptr<RequestHandler> handler_;
};

} // namespace servlets
} // namespace networking
