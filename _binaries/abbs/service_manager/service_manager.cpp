// Service Manager - Shared Library Build Management
//
// A focused tool for managing and rebuilding shared C++ libraries.
//
// Features:
// - Displays all .so/.dylib files in build/libraries/src/
// - Shows library details (size, last modified, make target)
// - Individual rebuild buttons for each library
// - Real-time library scanning
//
// Usage: ./service_manager [port]
// Default port: 9004
//
// Access at: http://localhost:9004

// Standard library headers
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <dlfcn.h>
#include <map>


#include "include/request_handlers.h"
// Project headers (organized)
// Core HTTP server and protocol
#include "../../../../_libraries/packages/IO/http_server/headers/http_server.h"
#include "../../../../_libraries/packages/IO/http_server/headers/HttpVersion.h"
#include "../../../../_libraries/packages/IO/http_server/headers/HttpMethod.h"
#include "../../../../_libraries/packages/IO/http_server/headers/default_handlers.h"

// Service manager core
#include "include/default_servlet.h"
#include "include/server_constants.hpp"
#include "include/utils.hpp"
#include "advanced_logging.h"
#include "json.h"
#include "request_response.h"

// Static assets
#include "static_assets/service_manager_html.h"

// Utilities
#include "MISC/unix_commands/unix_commands.hpp"
#include "MISC/elf_management/binary_info.hpp"
#include "MISC/elf_management/shared_library.hpp"

// Make help cache
#include "make_help_cache.hpp"
static MakeHelpCache make_help_cache;
struct MakeHelpCacheRefresher {
    MakeHelpCacheRefresher() { make_help_cache.refresh(); }
} make_help_cache_refresher;



int main(int argc, char* argv[]) {
    int port = 9004;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port argument, using default 9004." << std::endl;
        }
    }
    std::string proc = UnixCommands::get_process_on_port(port);
    if (!proc.empty()) {
        std::cerr << "Port " << port << " is already in use by process: " << proc << std::endl;
        std::cerr << "Would you like to try port " << (port + 1) << " instead? (y/n): ";
        char answer = 'n';
        std::cin >> answer;
        if (answer == 'y' || answer == 'Y') {
            port = port + 1;
            proc = UnixCommands::get_process_on_port(port);
            if (!proc.empty()) {
                std::cerr << "Port " << port << " is also in use by process: " << proc << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Exiting." << std::endl;
            return 1;
        }
    }

    advanced_logging::Logger logger("");

    // Create a default servlet for the server
    auto default_servlet = make_default_servlet(nullptr);
    ::io::http_server::HttpServer server(port, 4, &logger, default_servlet);

    // Use handlers from request_handlers.h
    auto make_help_handler = ::service_manager::make_help_handler();
    auto html_handler = ::service_manager::html_handler();
    auto test_handler = ::service_manager::test_handler();

    server.add_request_handler(html_handler);
    server.add_request_handler(test_handler);
    server.add_request_handler(make_help_handler);

    // Register generic embedded asset handler (serves all embedded assets)
    server.add_request_handler(::service_manager::embedded_asset_handler());

    // Display banner with all available routes
    server.display_banner();

    // Helper to prepend prefix to path
    auto make_handler = [](auto fn, auto method, std::string path, const std::string& prefix = "") {
        return ::io::http_server::RequestHandle::build(
            [fn](const std::string& req_str) -> Response {
                return fn(Request::from_string(req_str));
            },
            method,
            prefix + path
        );
    };

    std::string api_prefix = "/api";
    server.add_request_handler_group(
        make_handler(::service_manager::handle_demos, ::io::http_server::HttpMethod::GET, "/demos", api_prefix),
        make_handler(::service_manager::handle_services, ::io::http_server::HttpMethod::GET, "/services", api_prefix),
        make_handler(::service_manager::handle_apps, ::io::http_server::HttpMethod::GET, "/apps", api_prefix),
        make_handler(::service_manager::handle_libraries, ::io::http_server::HttpMethod::GET, "/libraries", api_prefix),
        make_handler(::service_manager::handle_binaries, ::io::http_server::HttpMethod::GET, "/binaries", api_prefix),
        make_handler(::service_manager::handle_libdocs, ::io::http_server::HttpMethod::GET, "/libdocs", api_prefix),
        make_handler(::service_manager::handle_routes, ::io::http_server::HttpMethod::GET, "/routes", api_prefix)
    );

    server.start();
    return 0;
}