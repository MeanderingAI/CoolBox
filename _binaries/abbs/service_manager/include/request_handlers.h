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



// Dynamic handler for any embedded static asset
io::http_server::RequestHandle embedded_asset_handler();


// Forward declarations for API handler functions (from utils.hpp)
std::string handle_api_routes();
std::string handle_api_binaries(const std::string& workspace_path);
std::string handle_api_libraries(const std::string& workspace_path);
std::string handle_api_libdocs(const std::string& libdocs_path);
std::string handle_api_demos();
std::string handle_api_services();
std::string handle_api_apps();
std::string handle_api_ui();
std::string handle_api_rebuild(const std::string& workspace_path, const std::string& target);
std::string handle_api_docs(const std::string& request);
std::string handle_api_docs_rebuild();






} // namespace service_manager

// StaticHandlers namespace for static file serving (global, not inside service_manager)
#include <memory>
namespace networking { namespace servlets { class RequestHandler; } }
namespace StaticHandlers {
	std::shared_ptr<networking::servlets::RequestHandler> make_static_file_handler(const std::string& static_prefix);
}
