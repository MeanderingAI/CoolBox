#include "rest_api/server.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>

int main() {
    using namespace ml::rest_api;
    Server server(0);
    server.register_handler("users.get", [](const Request& request) {
        Response response;
        response.set_body(request.get_path_param("id") + ":" +
                          request.get_query_param("active", "missing"));
        return response;
    });
    server.register_handler("users.me", [](const Request&) {
        return Response(200, "me");
    });
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto routes_path = std::filesystem::temp_directory_path() /
        ("coolbox_test_" + std::to_string(unique) + ".routes");
    {
        std::ofstream routes(routes_path);
        routes << "# One handler name can be shared by all language wrappers\n"
               << "GET /users/:id users.get\n"
               << "GET /users/me users.me\n";
    }
    server.load_routes(routes_path.string());
    std::filesystem::remove(routes_path);

    const Request request(HttpMethod::GET, "/users/42?active=true", {}, "");
    const Response response = server.handle_request(request);
    assert(response.status_code() == 200);
    assert(response.body() == "42:true");
    assert(server.handle_request(Request(HttpMethod::GET, "/users/me", {}, "")).body() == "me");
    assert(server.handle_request(Request(HttpMethod::GET, "/missing", {}, "")).status_code() == 404);
}
