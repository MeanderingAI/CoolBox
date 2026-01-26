#pragma once
#include "request_response.h"
#include "request_handle.h"
#include <string>

namespace service_manager {

// Handler declarations
io::http_server::RequestHandle make_help_handler();
io::http_server::RequestHandle html_handler();
io::http_server::RequestHandle test_handler();

// API handler declarations
Response handle_demos(const Request& req);
Response handle_services(const Request& req);
Response handle_apps(const Request& req);
Response handle_routes(const Request& req);
Response handle_binaries(const Request& req);
Response handle_libdocs(const Request& req);
Response handle_libraries(const Request& req);
Response handle_rebuild(const Request& req);
Response handle_docs_rebuild(const Request& req);
Response handle_ui(const Request& req);
Response handle_docs(const Request& req);

// Per-asset static handlers
io::http_server::RequestHandle service_manager_js_handler();
io::http_server::RequestHandle service_manager_css_handler();
io::http_server::RequestHandle make_help_tables_js_handler();
io::http_server::RequestHandle make_help_table_js_handler();
io::http_server::RequestHandle notification_center_js_handler();

} // namespace service_manager
