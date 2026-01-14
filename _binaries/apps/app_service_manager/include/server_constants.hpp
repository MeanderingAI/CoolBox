

#ifndef SERVICE_MANAGER_SERVER_CONSTANTS_HPP
#define SERVICE_MANAGER_SERVER_CONSTANTS_HPP

#include "dataformats/http/route.h"
#include "request_handlers.h"
#include <vector>

namespace dh = dataformats::http;
#define ROUTE(method, path, desc, handler) dh::Route{method, path, desc, handler}

namespace service_manager {

inline const std::vector<dh::Route>& get_api_routes() {
    static const std::vector<dh::Route> routes = {
        ROUTE("GET", "/api/routes", "List all available API routes", handle_routes),
        ROUTE("GET", "/api/binaries", "List all generated binaries in build/bin", handle_binaries),
        ROUTE("GET", "/api/libdocs", "List all available library documentation HTML index files", handle_libdocs),
        ROUTE("GET", "/api/libraries", "List all shared libraries and their metadata", handle_libraries),
        ROUTE("GET", "/api/demos", "List all demo binaries", handle_demos),
        ROUTE("GET", "/api/services", "List all services", handle_services),
        ROUTE("GET", "/api/apps", "List all apps", handle_apps),
        ROUTE("POST", "/api/rebuild", "Rebuild a specific shared library by target name", handle_rebuild),
        ROUTE("POST", "/api/docs-rebuild", "Regenerate all API documentation", handle_docs_rebuild),
        ROUTE("GET", "/", "Service manager web UI", handle_ui),
        ROUTE("GET", "/index", "Service manager web UI (index)", handle_ui),
        ROUTE("GET", "/docs/*", "Serve generated documentation HTML files", handle_docs)
    };
    return routes;
}

} // namespace service_manager

#endif // SERVICE_MANAGER_SERVER_CONSTANTS_HPP
