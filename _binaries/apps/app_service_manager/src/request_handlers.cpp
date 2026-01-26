


#include "request_handlers.h"
#include "make_help_cache.hpp"
#include "utils.hpp"
#include "static_assets/service_manager_html.h"
#include "static_assets/service_manager_js.h"
#include "static_assets/service_manager_css.h"
#include "static_assets/make_help_tables_js.h"
#include "static_assets/make_help_table_js.h"
#include "static_assets/notification_center_js.h"
#include "IO/advanced_logging/headers/advanced_logging.h"

#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include "../../../../_libraries/packages/IO/servlets/headers/http_servlet_base.h"

namespace StaticHandlers {
    class StaticFileRequestHandler : public networking::servlets::RequestHandler {
    public:
        explicit StaticFileRequestHandler(std::string static_prefix)
            : static_prefix_(std::move(static_prefix)) {}

        Response handle(const Request& request) override {
            Response resp;
            if (request.uri == "/") {
                resp.status_code = 200;
                resp.body = resources::service_manager_html;
                resp.headers["Content-Type"] = "text/html";
            } else if (request.uri.find(static_prefix_) == 0) {
                std::string rel_path = request.uri.substr(static_prefix_.size());
                std::string file_path = static_prefix_.substr(1) + rel_path; // Remove leading '/'
                std::ifstream file(file_path, std::ios::binary);
                if (file) {
                    std::ostringstream ss;
                    ss << file.rdbuf();
                    resp.body = ss.str();
                    resp.status_code = 200;
                    auto ext_pos = file_path.find_last_of('.');
                    std::string ext = (ext_pos != std::string::npos) ? file_path.substr(ext_pos + 1) : "";
                    std::string mime = "application/octet-stream";
                    if (ext == "html" || ext == "htm") mime = "text/html";
                    else if (ext == "css") mime = "text/css";
                    else if (ext == "js" || ext == "mjs") mime = "application/javascript";
                    else if (ext == "json") mime = "application/json";
                    else if (ext == "png") mime = "image/png";
                    else if (ext == "jpg" || ext == "jpeg") mime = "image/jpeg";
                    else if (ext == "gif") mime = "image/gif";
                    else if (ext == "svg") mime = "image/svg+xml";
                    else if (ext == "ico") mime = "image/x-icon";
                    else if (ext == "txt") mime = "text/plain";
                    else if (ext == "wasm") mime = "application/wasm";
                    else if (ext == "pdf") mime = "application/pdf";
                    else if (ext == "csv") mime = "text/csv";
                    resp.headers["Content-Type"] = mime;
                } else {
                    resp.status_code = 404;
                    resp.body = "<html><body><h1>404 Not Found</h1></body></html>";
                    resp.headers["Content-Type"] = "text/html";
                }
            } else {
                resp.status_code = 404;
                resp.body = "<html><body><h1>404 Not Found</h1></body></html>";
                resp.headers["Content-Type"] = "text/html";
            }
            return resp;
        }
    private:
        std::string static_prefix_;
    };

    std::shared_ptr<networking::servlets::RequestHandler> make_static_file_handler(const std::string& static_prefix) {
        return std::make_shared<StaticFileRequestHandler>(static_prefix);
    }
}

namespace HtmlHandlers {
    io::http_server::RequestHandle html_handler() {
        return io::http_server::RequestHandle::build(
            [](const std::string&) -> Response {
                Response resp;
                resp.status_code = 200;
                resp.body = resources::service_manager_html;
                resp.headers[HeaderKey::ContentType] = "text/html; charset=utf-8";
                resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
                resp.headers[HeaderKey::Connection] = "close";
                return resp;
            },
            io::http_server::HttpMethod::GET,
            "/"
        );
    }
}

namespace UtilityHandlers {
    io::http_server::RequestHandle test_handler() {
        return io::http_server::RequestHandle::build(
            [](const std::string&) -> Response {
                Response resp;
                resp.status_code = 200;
                resp.body = "ok";
                resp.headers[HeaderKey::ContentType] = "text/plain; charset=utf-8";
                resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
                resp.headers[HeaderKey::Connection] = "close";
                return resp;
            },
            io::http_server::HttpMethod::GET,
            "/test"
        );
    }
}

namespace ApiHandlers {
    io::http_server::RequestHandle make_help_handler() {
        return io::http_server::RequestHandle::build(
            [](const std::string&) -> Response {
                static MakeHelpCache make_help_cache;
                Response resp;
                resp.status_code = 200;
                resp.body = make_help_cache.raw;
                resp.headers[HeaderKey::ContentType] = "text/html; charset=utf-8";
                resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
                resp.headers[HeaderKey::Connection] = "close";
                return resp;
            },
            io::http_server::HttpMethod::GET,
            "/make_help"
        );
    }

    // Add other API handlers here (handle_demos, handle_services, etc.)
    // Example:
    // io::http_server::RequestHandle demos_handler() { ... }
}

namespace service_manager {

io::http_server::RequestHandle make_help_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string&) -> Response {
            static MakeHelpCache make_help_cache;
            Response resp;
            resp.status_code = 200;
            resp.body = make_help_cache.raw;
            resp.headers[HeaderKey::ContentType] = "text/html; charset=utf-8";
            resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
            resp.headers[HeaderKey::Connection] = "close";
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/make_help"
    );
}

static MakeHelpCache make_help_cache;



io::http_server::RequestHandle html_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string&) -> Response {
            Response resp;
            resp.status_code = 200;
            resp.body = resources::service_manager_html;
            resp.headers[HeaderKey::ContentType] = "text/html; charset=utf-8";
            resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
            resp.headers[HeaderKey::Connection] = "close";
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/"
    );
}

io::http_server::RequestHandle test_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string&) -> Response {
            Response resp;
            resp.status_code = 200;
            resp.body = "ok";
            resp.headers[HeaderKey::ContentType] = "text/plain; charset=utf-8";
            resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
            resp.headers[HeaderKey::Connection] = "close";
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/test"
    );
}

// Per-asset static handlers
io::http_server::RequestHandle service_manager_js_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string&) -> Response {
            Response resp;
            resp.status_code = 200;
            resp.body = resources::service_manager_js;
            resp.headers[HeaderKey::ContentType] = "application/javascript; charset=utf-8";
            resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
            resp.headers[HeaderKey::Connection] = "close";
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/_static_assets/resources/js/service_manager.js"
    );
}

io::http_server::RequestHandle service_manager_css_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string&) -> Response {
            Response resp;
            resp.status_code = 200;
            resp.body = resources::service_manager_css;
            resp.headers[HeaderKey::ContentType] = "text/css; charset=utf-8";
            resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
            resp.headers[HeaderKey::Connection] = "close";
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/_static_assets/resources/css/service_manager.css"
    );
}

io::http_server::RequestHandle make_help_tables_js_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string&) -> Response {
            Response resp;
            resp.status_code = 200;
                resp.body = resources::make_help_tables_js;
            resp.headers[HeaderKey::ContentType] = "application/javascript; charset=utf-8";
            resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
            resp.headers[HeaderKey::Connection] = "close";
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/_static_assets/resources/js/make-help-tables.js"
    );
}

io::http_server::RequestHandle make_help_table_js_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string&) -> Response {
            Response resp;
            resp.status_code = 200;
                resp.body = resources::make_help_table_js;
            resp.headers[HeaderKey::ContentType] = "application/javascript; charset=utf-8";
            resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
            resp.headers[HeaderKey::Connection] = "close";
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/_static_assets/resources/js/make-help-table.js"
    );
}

io::http_server::RequestHandle notification_center_js_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string&) -> Response {
            Response resp;
            resp.status_code = 200;
                resp.body = resources::notification_center_js;
            resp.headers[HeaderKey::ContentType] = "application/javascript; charset=utf-8";
            resp.headers[HeaderKey::ContentLength] = std::to_string(resp.body.size());
            resp.headers[HeaderKey::Connection] = "close";
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/_static_assets/resources/js/notification-center.js"
    );
}

// API handler definitions (restored from .bak)
Response handle_demos(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_demos();
    return resp;
}

Response handle_services(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_services();
    return resp;
}

Response handle_apps(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_apps();
    return resp;
}

Response handle_routes(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_routes();
    return resp;
}

Response handle_binaries(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_binaries(req.body);
    return resp;
}

Response handle_libdocs(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_libdocs("gen_docs/html/libs");
    return resp;
}

Response handle_libraries(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_libraries(".");
    return resp;
}

Response handle_rebuild(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_rebuild(".", req.body);
    return resp;
}

Response handle_docs_rebuild(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_docs_rebuild();
    return resp;
}

Response handle_ui(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "text/html";
    resp.body = handle_api_ui();
    return resp;
}

Response handle_docs(const Request& req) {
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "text/html";
    resp.body = handle_api_docs(req.uri);
    return resp;
}

} // namespace service_manager
