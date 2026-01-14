



#include "request_handlers.h"
#include "utils.hpp"
#include "dataformats/json/json.h"
#include <string>
#include "advanced_logging/advanced_logging.h"

// Explicit externs to guarantee visibility (workaround for include path issues)
extern std::string handle_api_demos();
extern std::string handle_api_services();
extern std::string handle_api_apps();


namespace nhh = networking::http;

namespace service_manager {

static advanced_logging::Logger logger("service_manager_requests.log");

nhh::Response handle_demos(const nhh::Request& req) {
    logger.info("[API] /api/demos called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_demos());
    return resp;
}

nhh::Response handle_services(const nhh::Request& req) {
    logger.info("[API] /api/services called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_services());
    return resp;
}

nhh::Response handle_apps(const nhh::Request& req) {
    logger.info("[API] /api/apps called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_apps());
    return resp;
}

nhh::Response handle_routes(const nhh::Request& req) {
    logger.info("[API] /api/routes called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_routes());
    return resp;
}

nhh::Response handle_binaries(const nhh::Request& req) {
    logger.info("[API] /api/binaries called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_binaries("."));
    return resp;
}

nhh::Response handle_libdocs(const nhh::Request& req) {
    logger.info("[API] /api/libdocs called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_libdocs("gen_docs/html/libs"));
    return resp;
}

nhh::Response handle_libraries(const nhh::Request& req) {
    logger.info("[API] /api/libraries called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_libraries("."));
    return resp;
}

nhh::Response handle_rebuild(const nhh::Request& req) {
    logger.info("[API] /api/rebuild called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_rebuild(".", req.body()));
    return resp;
}

nhh::Response handle_docs_rebuild(const nhh::Request& req) {
    logger.info("[API] /api/docs_rebuild called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "application/json");
    resp.set_body(handle_api_docs_rebuild());
    return resp;
}

nhh::Response handle_ui(const nhh::Request& req) {
    logger.info("[API] / called");
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "text/html");
    resp.set_body(handle_api_ui());
    return resp;
}

nhh::Response handle_docs(const nhh::Request& req) {
    logger.info("[API] /docs called, path: " + req.path());
    nhh::Response resp;
    resp.set_status(nhh::HttpStatus::OK);
    resp.set_header("Content-Type", "text/html");
    resp.set_body(handle_api_docs(req.path()));
    return resp;
}

} // namespace service_manager
