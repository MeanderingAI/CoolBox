#ifndef SERVICE_MANAGER_REQUEST_HANDLERS_H
#define SERVICE_MANAGER_REQUEST_HANDLERS_H

#include "networking/rest_api/server.h"
#include "networking/http/request_response.h"
#include "dataformats/json/json.h"
#include <vector>
#include <string>


namespace nhh = networking::http;

namespace service_manager {

nhh::Response handle_routes(const nhh::Request& req);
nhh::Response handle_binaries(const nhh::Request& req);
nhh::Response handle_libdocs(const nhh::Request& req);
nhh::Response handle_libraries(const nhh::Request& req);
nhh::Response handle_demos(const nhh::Request& req);
nhh::Response handle_services(const nhh::Request& req);
nhh::Response handle_apps(const nhh::Request& req);
nhh::Response handle_rebuild(const nhh::Request& req);
nhh::Response handle_docs_rebuild(const nhh::Request& req);
nhh::Response handle_ui(const nhh::Request& req); // Serves / and /index
nhh::Response handle_docs(const nhh::Request& req);

} // namespace service_manager

#endif // SERVICE_MANAGER_REQUEST_HANDLERS_H
