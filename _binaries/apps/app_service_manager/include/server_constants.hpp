#ifndef SERVICE_MANAGER_SERVER_CONSTANTS_HPP
#define SERVICE_MANAGER_SERVER_CONSTANTS_HPP

#include "route.h"
#include "request_handlers.h"
#include <vector>

namespace dh = dataformats::http;

// Helper macro to wrap string-returning handlers into the expected signature
#define ROUTE(method, path, desc, handler) dh::Route{method, path, desc, handler}

// Helper lambdas for each handler
#define HANDLER_API_ROUTES [] (const Request&) -> Response { return Response::ok(handle_api_routes()); }
#define HANDLER_API_BINARIES [] (const Request& req) -> Response { return Response::ok(handle_api_binaries("")); }
#define HANDLER_API_LIBDOCS [] (const Request& req) -> Response { return Response::ok(handle_api_libdocs("")); }
#define HANDLER_API_LIBRARIES [] (const Request& req) -> Response { return Response::ok(handle_api_libraries("")); }
#define HANDLER_API_DEMOS [] (const Request&) -> Response { return Response::ok(handle_api_demos()); }
#define HANDLER_API_SERVICES [] (const Request&) -> Response { return Response::ok(handle_api_services()); }
#define HANDLER_API_APPS [] (const Request&) -> Response { return Response::ok(handle_api_apps()); }
#define HANDLER_API_UI [] (const Request&) -> Response { return Response::ok(handle_api_ui()); }
#define HANDLER_API_REBUILD [] (const Request&) -> Response { return Response::ok(handle_api_rebuild("", "")); }
#define HANDLER_API_DOCS [] (const Request&) -> Response { return Response::ok(handle_api_docs("")); }
#define HANDLER_API_DOCS_REBUILD [] (const Request&) -> Response { return Response::ok(handle_api_docs_rebuild()); }


namespace service_manager {

inline const std::vector<dh::Route>& get_api_routes() {
    static const std::vector<dh::Route> routes = {
        ROUTE("GET", "/api/routes", "List all available API routes", HANDLER_API_ROUTES),
        ROUTE("GET", "/api/binaries", "List all generated binaries in build/bin", HANDLER_API_BINARIES),
        ROUTE("GET", "/api/libdocs", "List all available library documentation HTML index files", HANDLER_API_LIBDOCS),
        ROUTE("GET", "/api/libraries", "List all shared libraries and their metadata", HANDLER_API_LIBRARIES),
        ROUTE("GET", "/api/demos", "List all demo binaries", HANDLER_API_DEMOS),
        ROUTE("GET", "/api/services", "List all services", HANDLER_API_SERVICES),
        ROUTE("GET", "/api/apps", "List all apps", HANDLER_API_APPS),
        ROUTE("POST", "/api/rebuild", "Rebuild a specific shared library by target name", HANDLER_API_REBUILD),
        ROUTE("POST", "/api/docs-rebuild", "Regenerate all API documentation", HANDLER_API_DOCS_REBUILD),
        ROUTE("GET", "/", "Service manager web UI", HANDLER_API_UI),
        ROUTE("GET", "/index", "Service manager web UI (index)", HANDLER_API_UI),
        ROUTE("GET", "/docs/*", "Serve generated documentation HTML files", HANDLER_API_DOCS)
    };
    return routes;
}

} // namespace service_manager

#endif // SERVICE_MANAGER_SERVER_CONSTANTS_HPP
