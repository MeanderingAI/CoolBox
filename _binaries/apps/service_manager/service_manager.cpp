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

// Project headers
#include "networking/rest_api/unified_http_server.h"
#include "dataformats/json/json.h"
#include "networking/http/request_response.h"
#include "utils.hpp"
#include "request_handlers.h"
#include "server_constants.hpp"

#include "binary_info.hpp"
#include "shared_library.hpp"
#include "advanced_logging/advanced_logging.h"



int main(int argc, char* argv[]) {
    static advanced_logging::Logger logger("service_manager.log");
    int port = 9004;
    size_t num_threads = 8;
    std::string config_path = "config/path_routes/bp.json";
    static handler_settings settings = handler_settings::from_config(config_path);
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port < 1024 || port > 65535) port = 9004;
    }



    const auto& routes = service_manager::get_api_routes();

    logger.info("Exposed API Routes:");
    for (const auto& r : routes) {
        logger.info("  [" + r.method + "] " + r.path + " - " + r.description);
    }

    std::vector<std::shared_ptr<networking::rest_api::Route>> shared_routes;
    for (const auto& r : routes) {
        networking::http::HttpMethod method_enum = networking::http::HttpMethod::GET;
        if (r.method == "GET") method_enum = networking::http::HttpMethod::GET;
        else if (r.method == "POST") method_enum = networking::http::HttpMethod::POST;
        else if (r.method == "PUT") method_enum = networking::http::HttpMethod::PUT;
        else if (r.method == "DELETE" || r.method == "DELETE_") method_enum = networking::http::HttpMethod::DELETE_;
        else if (r.method == "PATCH") method_enum = networking::http::HttpMethod::PATCH;
        else if (r.method == "OPTIONS") method_enum = networking::http::HttpMethod::OPTIONS;
        shared_routes.push_back(std::make_shared<networking::rest_api::Route>(
            r.path, method_enum, r.handler));
    }

    networking::rest_api::UnifiedHttpServer unified_server(port, num_threads);
    unified_server.enable_cors();
    unified_server.load_routes(shared_routes);
    unified_server.start();
    logger.info("Unified HTTP server running on port " + std::to_string(port) + " (HTTP/1.1, HTTP/2, HTTP/3)");
    logger.debug("[DEBUG] After unified_server.start(), is_running() = " + std::string(unified_server.is_running() ? "true" : "false"));
    int loop_count = 0;
    while (unified_server.is_running()) {
        logger.debug("[DEBUG] Main loop, is_running() = true, loop_count = " + std::to_string(loop_count++));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    logger.warn("[DEBUG] Main loop exited, is_running() = " + std::string(unified_server.is_running() ? "true" : "false"));

    return 0;
}
