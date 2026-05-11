

#include "static_assets/app_launcher_html.h"
#include "static_assets/frontend_manager_html.h"
#include "static_assets/service_manager_html.h"
#include "static_assets/service_manager_js.h"
#include "static_assets/service_manager_css.h"
#include "static_assets/make_help_tables_js.h"
#include "static_assets/make_help_table_js.h"
#include "static_assets/notification_center_js.h"
#include "request_handlers.h"
#include "make_help_cache.hpp"
#include "utils.hpp"
#include "IO/advanced_logging/headers/advanced_logging.h"
#include "request_response.h"
#include <unordered_map>
#include <string_view>
#include <map>
#include <cstring>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include "../../../../_libraries/packages/IO/servlets/headers/http_servlet_base.h"

namespace service_manager {

#define EMBEDDED_ASSET_LIST \
    X("/_static_assets/resources/html/app_launcher.html", resources::app_launcher_html, "text/html") \
    X("/_static_assets/resources/html/frontend_manager.html", resources::frontend_manager_html, "text/html") \
    X("/_static_assets/resources/html/service_manager.html", resources::service_manager_html, "text/html") \
    X("/_static_assets/resources/js/service_manager.js", resources::service_manager_js, "application/javascript") \
    X("/_static_assets/resources/js/make-help-table.js", resources::make_help_table_js, "application/javascript") \
    X("/_static_assets/resources/js/make-help-tables.js", resources::make_help_tables_js, "application/javascript") \
    X("/_static_assets/resources/js/notification-center.js", resources::notification_center_js, "application/javascript") \
    X("/_static_assets/resources/css/service_manager.css", resources::service_manager_css, "text/css")

namespace {
struct EmbeddedAsset {
    const char* data;
    const char* content_type;
};

const std::map<std::string_view, EmbeddedAsset> embedded_assets = [] {
    std::map<std::string_view, EmbeddedAsset> m;
#define X(url, var, ctype) m.emplace(url, EmbeddedAsset{var, ctype});
    EMBEDDED_ASSET_LIST
#undef X
    return m;
}();
}

io::http_server::RequestHandle embedded_asset_handler() {
    return io::http_server::RequestHandle::build(
        [](const std::string& req_str) -> Response {
            Request req = Request::from_string(req_str);
            Response resp;
            auto it = embedded_assets.find(req.uri);
            if (it != embedded_assets.end()) {
                resp.status_code = 200;
                resp.body = it->second.data;
                resp.headers["Content-Type"] = it->second.content_type;
            } else {
                resp.status_code = 404;
                resp.body = "<html><body><h1>404 Not Found</h1></body></html>";
                resp.headers["Content-Type"] = "text/html";
            }
            return resp;
        },
        io::http_server::HttpMethod::GET,
        "/_static_assets/resources/"
    );
}

} // namespace service_manager



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
