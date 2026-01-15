#ifndef SERVICE_MANAGER_REQUEST_HANDLERS_H
#define SERVICE_MANAGER_REQUEST_HANDLERS_H

#include "IO/servlets/http_server.h"
#include "IO/servlets/HttpVersion.h"
#include "IO/dataformats/http/request_response.h"
#include "IO/dataformats/json/json.h"
#include <vector>
#include <string>


namespace service_manager {

Response handle_routes(const Request& req);
Response handle_binaries(const Request& req);
Response handle_libdocs(const Request& req);
Response handle_libraries(const Request& req);
Response handle_demos(const Request& req);
Response handle_services(const Request& req);
Response handle_apps(const Request& req);
Response handle_rebuild(const Request& req);
Response handle_docs_rebuild(const Request& req);
Response handle_ui(const Request& req); // Serves / and /index
Response handle_docs(const Request& req);

} // namespace service_manager

#endif // SERVICE_MANAGER_REQUEST_HANDLERS_H
