#include "request_handlers.h"
#include "utils.hpp"
#include "IO/dataformats/json/json.h"
#include <string>
#include "IO/advanced_logging/advanced_logging.h"

// Explicit externs to guarantee visibility (workaround for include path issues)
extern std::string handle_api_demos();
extern std::string handle_api_services();
extern std::string handle_api_apps();


namespace service_manager {

static advanced_logging::Logger logger("service_manager_requests.log");

Response handle_demos(const Request& req) {
    logger.info("[API] /api/demos called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_demos();
    return resp;
}

Response handle_services(const Request& req) {
    logger.info("[API] /api/services called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_services();
    return resp;
}

Response handle_apps(const Request& req) {
    logger.info("[API] /api/apps called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_apps();
    return resp;
}

Response handle_routes(const Request& req) {
    logger.info("[API] /api/routes called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_routes();
    return resp;
}

Response handle_binaries(const Request& req) {
    logger.info("[API] /api/binaries called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_binaries(".");
    return resp;
}

Response handle_libdocs(const Request& req) {
    logger.info("[API] /api/libdocs called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_libdocs("gen_docs/html/libs");
    return resp;
}

Response handle_libraries(const Request& req) {
    logger.info("[API] /api/libraries called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_libraries(".");
    return resp;
}

Response handle_rebuild(const Request& req) {
    logger.info("[API] /api/rebuild called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_rebuild(".", req.body);
    return resp;
}

Response handle_docs_rebuild(const Request& req) {
    logger.info("[API] /api/docs_rebuild called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "application/json";
    resp.body = handle_api_docs_rebuild();
    return resp;
}

Response handle_ui(const Request& req) {
    logger.info("[API] / called");
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "text/html";
    resp.body = handle_api_ui();
    return resp;
}

Response handle_docs(const Request& req) {
    logger.info("[API] /docs called, path: " + req.uri);
    Response resp;
    resp.status_code = 200;
    resp.headers["Content-Type"] = "text/html";
    resp.body = handle_api_docs(req.uri);
    return resp;
}

} // namespace service_manager
