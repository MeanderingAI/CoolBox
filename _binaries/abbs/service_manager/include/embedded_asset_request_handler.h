
#pragma once
#include "../../../../_libraries/packages/IO/servlets/headers/http_servlet_base.h"
#include "request_response.h"
#include <memory>
#include <string>
#include <utility>

namespace service_manager {

// Adapter to wrap a RequestHandle as a RequestHandler
class EmbeddedAssetRequestHandler : public networking::servlets::RequestHandler {
public:
    explicit EmbeddedAssetRequestHandler(io::http_server::RequestHandle handle)
        : handle_(std::move(handle)) {}
    Response handle(const Request& request) override {
        // The RequestHandle expects the raw request string, so re-serialize if needed
        // But here, we don't have the original string, so this adapter is not directly compatible.
        // Instead, we should adapt the handler to work with the Request object directly, or refactor the handler signature.
        // For now, return a 500 error to indicate this is not supported.
        Response resp;
        resp.status_code = 500;
        resp.body = "<html><body><h1>500 Internal Server Error</h1><p>Request adapter not implemented.</p></body></html>";
        resp.headers["Content-Type"] = "text/html";
        return resp;
    }
private:
    io::http_server::RequestHandle handle_;
};

inline std::shared_ptr<networking::servlets::RequestHandler> make_embedded_asset_request_handler() {
    return std::make_shared<EmbeddedAssetRequestHandler>(embedded_asset_handler());
}

} // namespace service_manager
