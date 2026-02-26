/**
 * Basic REST API Server Example
 * 
 * Demonstrates:
 * - Creating a server
 * - Adding GET and POST routes
 * - Handling JSON requests/responses
 * - Thread pool usage
 */

#include "networking/rest_api/server.h"
#include "dataformats/json/json.h"
#include <iostream>

using namespace networking::rest_api;
using namespace networking::json;

int main() {
    // Create server with 4 threads
    Server server(8080, 4);
    
    // GET route: Hello world
    server.get("/", [](const Request& req) {
        Response res;
        res.set_status(HttpStatus::OK);
        res.set_json("{\"message\": \"Hello, World!\"}");
        return res;
    });
    
    // GET route with path parameter
    server.get("/users/:id", [](const Request& req) {
        auto params = req.path_params();
        auto it = params.find("id");
        
        Response res;
        if (it != params.end()) {
            Builder json_builder;
            json_builder.add("user_id", it->second)
                       .add("name", "User " + it->second)
                       .add("status", "active");
            
            res.set_status(HttpStatus::OK);
            res.set_json(json_builder.build().to_string());
        } else {
            res.set_status(HttpStatus::BAD_REQUEST);
            res.set_json("{\"error\": \"Missing user ID\"}");
        }
        return res;
    });
    
    // POST route: Create user
    server.post("/users", [](const Request& req) {
        try {
            // Parse request body
            Parser parser(req.body());
            Value json_data = parser.parse();
            
            // Build response
            Builder response_builder;
            response_builder.add("message", "User created")
                          .add("id", "12345")
                          .add("data", json_data);
            
            Response res;
            res.set_status(HttpStatus::CREATED);
            res.set_json(response_builder.build().to_string());
            return res;
        } catch (const std::exception& e) {
            Response res;
            res.set_status(HttpStatus::BAD_REQUEST);
            res.set_json("{\"error\": \"Invalid JSON\"}");
            return res;
        }
    });
    
    // Demonstrate synchronous request handling
    std::cout << "=== Basic Server Example ===" << std::endl;
    server.start();
    
    // Test GET /
    Request req1;
    req1.set_method("GET");
    req1.set_path("/");
    Response res1 = server.handle_request(req1);
    std::cout << "\nGET / => " << res1.body() << std::endl;
    
    // Test GET /users/42
    Request req2;
    req2.set_method("GET");
    req2.set_path("/users/42");
    Response res2 = server.handle_request(req2);
    std::cout << "GET /users/42 => " << res2.body() << std::endl;
    
    // Test POST /users
    Request req3;
    req3.set_method("POST");
    req3.set_path("/users");
    req3.set_body("{\"name\": \"John Doe\", \"email\": \"john@example.com\"}");
    Response res3 = server.handle_request(req3);
    std::cout << "POST /users => " << res3.body() << std::endl;
    
    server.stop();
    return 0;
}
