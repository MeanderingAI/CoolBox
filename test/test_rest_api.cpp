#include <gtest/gtest.h>
#include "networking/rest_api/server.h"
#include "networking/rest_api/http_protocol.h"
#include "networking/rest_api/http_server_base.h"
#include "networking/rest_api/http1_servlet.h"
#include "networking/rest_api/http2_server.h"
#include "networking/rest_api/http3_server.h"
#include "dataformats/json/json.h"

using namespace ml::rest_api;
using namespace dataformats::json;

// ============================================================================
// Request Tests
// ============================================================================

TEST(RequestTest, DefaultConstructor) {
    Request req;
    EXPECT_EQ(req.method(), "GET");
    EXPECT_EQ(req.path(), "/");
    EXPECT_TRUE(req.body().empty());
}

TEST(RequestTest, SetMethod) {
    Request req;
    req.set_method("POST");
    EXPECT_EQ(req.method(), "POST");
}

TEST(RequestTest, SetPath) {
    Request req;
    req.set_path("/api/users");
    EXPECT_EQ(req.path(), "/api/users");
}

TEST(RequestTest, SetBody) {
    Request req;
    req.set_body("{\"name\":\"John\"}");
    EXPECT_EQ(req.body(), "{\"name\":\"John\"}");
}

TEST(RequestTest, SetHeader) {
    Request req;
    req.set_header("Content-Type", "application/json");
    EXPECT_EQ(req.get_header("Content-Type"), "application/json");
}

TEST(RequestTest, GetHeaderNotFound) {
    Request req;
    EXPECT_EQ(req.get_header("Missing-Header"), "");
}

TEST(RequestTest, HasHeader) {
    Request req;
    req.set_header("Authorization", "Bearer token");
    EXPECT_TRUE(req.has_header("Authorization"));
    EXPECT_FALSE(req.has_header("Missing"));
}

TEST(RequestTest, PathParameters) {
    Request req;
    req.set_path("/users/123");
    req.set_path_param("id", "123");
    
    EXPECT_EQ(req.get_path_param("id"), "123");
    EXPECT_TRUE(req.has_path_param("id"));
}

TEST(RequestTest, QueryParameters) {
    Request req;
    req.set_query_param("page", "1");
    req.set_query_param("limit", "10");
    
    EXPECT_EQ(req.get_query_param("page"), "1");
    EXPECT_EQ(req.get_query_param("limit"), "10");
    EXPECT_TRUE(req.has_query_param("page"));
}

// ============================================================================
// Response Tests
// ============================================================================

TEST(ResponseTest, DefaultConstructor) {
    Response res;
    EXPECT_EQ(res.status(), HttpStatus::OK);
    EXPECT_TRUE(res.body().empty());
}

TEST(ResponseTest, SetStatus) {
    Response res;
    res.set_status(HttpStatus::NOT_FOUND);
    EXPECT_EQ(res.status(), HttpStatus::NOT_FOUND);
}

TEST(ResponseTest, SetBody) {
    Response res;
    res.set_body("Hello World");
    EXPECT_EQ(res.body(), "Hello World");
}

TEST(ResponseTest, SetJson) {
    Response res;
    res.set_json("{\"message\":\"success\"}");
    EXPECT_EQ(res.body(), "{\"message\":\"success\"}");
    EXPECT_EQ(res.get_header("Content-Type"), "application/json");
}

TEST(ResponseTest, SetHeader) {
    Response res;
    res.set_header("X-Custom", "value");
    EXPECT_EQ(res.get_header("X-Custom"), "value");
}

TEST(ResponseTest, HttpStatusCodes) {
    EXPECT_EQ(static_cast<int>(HttpStatus::OK), 200);
    EXPECT_EQ(static_cast<int>(HttpStatus::CREATED), 201);
    EXPECT_EQ(static_cast<int>(HttpStatus::BAD_REQUEST), 400);
    EXPECT_EQ(static_cast<int>(HttpStatus::NOT_FOUND), 404);
    EXPECT_EQ(static_cast<int>(HttpStatus::INTERNAL_SERVER_ERROR), 500);
}

// ============================================================================
// Server Tests
// ============================================================================

TEST(ServerTest, Construction) {
    Server server(8080, 4);
    EXPECT_EQ(server.port(), 8080);
    EXPECT_EQ(server.num_threads(), 4u);
}

TEST(ServerTest, AddGetRoute) {
    Server server(8080, 4);
    
    server.get("/", [](const Request& req) {
        Response res;
        res.set_status(HttpStatus::OK);
        res.set_body("Hello");
        return res;
    });
    
    Request req;
    req.set_method("GET");
    req.set_path("/");
    
    Response res = server.handle_request(req);
    EXPECT_EQ(res.status(), HttpStatus::OK);
    EXPECT_EQ(res.body(), "Hello");
}

TEST(ServerTest, AddPostRoute) {
    Server server(8080, 4);
    
    server.post("/api/data", [](const Request& req) {
        Response res;
        res.set_status(HttpStatus::CREATED);
        res.set_json("{\"status\":\"created\"}");
        return res;
    });
    
    Request req;
    req.set_method("POST");
    req.set_path("/api/data");
    req.set_body("{\"key\":\"value\"}");
    
    Response res = server.handle_request(req);
    EXPECT_EQ(res.status(), HttpStatus::CREATED);
    EXPECT_TRUE(res.body().find("created") != std::string::npos);
}

TEST(ServerTest, PathParameters) {
    Server server(8080, 4);
    
    server.get("/users/:id", [](const Request& req) {
        Response res;
        std::string id = req.get_path_param("id");
        
        Builder builder;
        builder.add("userId", id);
        
        res.set_json(builder.build().to_string());
        return res;
    });
    
    Request req;
    req.set_method("GET");
    req.set_path("/users/123");
    req.set_path_param("id", "123");
    
    Response res = server.handle_request(req);
    EXPECT_TRUE(res.body().find("123") != std::string::npos);
}

TEST(ServerTest, NotFoundRoute) {
    Server server(8080, 4);
    
    server.get("/exists", [](const Request& req) {
        Response res;
        res.set_body("Found");
        return res;
    });
    
    Request req;
    req.set_method("GET");
    req.set_path("/missing");
    
    Response res = server.handle_request(req);
    EXPECT_EQ(res.status(), HttpStatus::NOT_FOUND);
}

TEST(ServerTest, MethodNotAllowed) {
    Server server(8080, 4);
    
    server.get("/resource", [](const Request& req) {
        Response res;
        res.set_body("GET response");
        return res;
    });
    
    Request req;
    req.set_method("POST");
    req.set_path("/resource");
    
    Response res = server.handle_request(req);
    EXPECT_EQ(res.status(), HttpStatus::NOT_FOUND);
}

TEST(ServerTest, MultipleRoutes) {
    Server server(8080, 4);
    
    server.get("/route1", [](const Request& req) {
        Response res;
        res.set_body("Route 1");
        return res;
    });
    
    server.get("/route2", [](const Request& req) {
        Response res;
        res.set_body("Route 2");
        return res;
    });
    
    Request req1;
    req1.set_path("/route1");
    EXPECT_EQ(server.handle_request(req1).body(), "Route 1");
    
    Request req2;
    req2.set_path("/route2");
    EXPECT_EQ(server.handle_request(req2).body(), "Route 2");
}

// ============================================================================
// HTTP Protocol Tests
// ============================================================================

TEST(HttpProtocolTest, HttpVersionEnum) {
    EXPECT_EQ(http_version_to_string(HttpVersion::HTTP_1_0), "HTTP/1.0");
    EXPECT_EQ(http_version_to_string(HttpVersion::HTTP_1_1), "HTTP/1.1");
    EXPECT_EQ(http_version_to_string(HttpVersion::HTTP_2), "HTTP/2");
    EXPECT_EQ(http_version_to_string(HttpVersion::HTTP_3), "HTTP/3");
}

TEST(HttpProtocolTest, ProtocolCapabilities_HTTP1) {
    auto caps = ProtocolCapabilities::for_version(HttpVersion::HTTP_1_1);
    
    EXPECT_FALSE(caps.supports_multiplexing);
    EXPECT_FALSE(caps.supports_server_push);
    EXPECT_FALSE(caps.supports_header_compression);
    EXPECT_FALSE(caps.supports_prioritization);
    EXPECT_FALSE(caps.is_encrypted);
    EXPECT_FALSE(caps.is_udp_based);
}

TEST(HttpProtocolTest, ProtocolCapabilities_HTTP2) {
    auto caps = ProtocolCapabilities::for_version(HttpVersion::HTTP_2);
    
    EXPECT_TRUE(caps.supports_multiplexing);
    EXPECT_TRUE(caps.supports_server_push);
    EXPECT_TRUE(caps.supports_header_compression);
    EXPECT_TRUE(caps.supports_prioritization);
    EXPECT_TRUE(caps.is_encrypted);
    EXPECT_FALSE(caps.is_udp_based);
}

TEST(HttpProtocolTest, ProtocolCapabilities_HTTP3) {
    auto caps = ProtocolCapabilities::for_version(HttpVersion::HTTP_3);
    
    EXPECT_TRUE(caps.supports_multiplexing);
    EXPECT_TRUE(caps.supports_server_push);
    EXPECT_TRUE(caps.supports_header_compression);
    EXPECT_TRUE(caps.supports_prioritization);
    EXPECT_TRUE(caps.is_encrypted);
    EXPECT_TRUE(caps.is_udp_based);
}

TEST(HttpProtocolTest, HPACKEncoder) {
    HPACKEncoder encoder;
    
    std::map<std::string, std::string> headers;
    headers["content-type"] = "application/json";
    headers["user-agent"] = "TestAgent/1.0";
    
    std::vector<uint8_t> encoded = encoder.encode(headers);
    EXPECT_FALSE(encoded.empty());
    
    std::map<std::string, std::string> decoded = encoder.decode(encoded);
    EXPECT_EQ(decoded.size(), headers.size());
    EXPECT_EQ(decoded["content-type"], "application/json");
}

TEST(HttpProtocolTest, QPACKEncoder) {
    QPACKEncoder encoder;
    
    std::map<std::string, std::string> headers;
    headers["content-type"] = "text/html";
    headers["accept"] = "*/*";
    
    std::vector<uint8_t> encoded = encoder.encode(headers);
    EXPECT_FALSE(encoded.empty());
    
    std::map<std::string, std::string> decoded = encoder.decode(encoded);
    EXPECT_EQ(decoded.size(), headers.size());
}

// ============================================================================
// HTTP Server Factory Tests
// ============================================================================

TEST(HttpServerFactoryTest, CreateHTTP1) {
    auto server = HttpServerFactory::create_http1(8080, 4);
    
    EXPECT_NE(server, nullptr);
    EXPECT_EQ(server->protocol_version(), HttpVersion::HTTP_1_1);
    EXPECT_EQ(server->protocol_name(), "HTTP/1.1");
}

TEST(HttpServerFactoryTest, CreateHTTP2) {
    auto server = HttpServerFactory::create_http2(8081, 4);
    
    EXPECT_NE(server, nullptr);
    EXPECT_EQ(server->protocol_version(), HttpVersion::HTTP_2);
    EXPECT_EQ(server->protocol_name(), "HTTP/2");
}

TEST(HttpServerFactoryTest, CreateHTTP3) {
    auto server = HttpServerFactory::create_http3(8082, 4);
    
    EXPECT_NE(server, nullptr);
    EXPECT_EQ(server->protocol_version(), HttpVersion::HTTP_3);
    EXPECT_EQ(server->protocol_name(), "HTTP/3");
}

TEST(HttpServerFactoryTest, CreateByVersion) {
    auto http1 = HttpServerFactory::create(HttpVersion::HTTP_1_1, 8080, 4);
    auto http2 = HttpServerFactory::create(HttpVersion::HTTP_2, 8081, 4);
    auto http3 = HttpServerFactory::create(HttpVersion::HTTP_3, 8082, 4);
    
    EXPECT_EQ(http1->protocol_version(), HttpVersion::HTTP_1_1);
    EXPECT_EQ(http2->protocol_version(), HttpVersion::HTTP_2);
    EXPECT_EQ(http3->protocol_version(), HttpVersion::HTTP_3);
}

TEST(HttpServerFactoryTest, ServerRouting) {
    auto server = HttpServerFactory::create_http2(8080, 4);
    
    server->get("/test", [](const Request& req) {
        Response res;
        res.set_body("HTTP/2 Test");
        return res;
    });
    
    Request req;
    req.set_path("/test");
    
    Response res = server->handle_request(req);
    EXPECT_EQ(res.body(), "HTTP/2 Test");
}

// ============================================================================
// HTTP/1.1 Server Tests
// ============================================================================

TEST(Http1ServletTest, KeepAliveSettings) {
    Http1Servlet server(8080, 4);
    
    // Test default keep-alive
    server.set_keep_alive(true, 10);
    
    Request req;
    req.set_path("/");
    
    Response res = server.handle_request(req);
    EXPECT_EQ(res.status(), HttpStatus::NOT_FOUND); // No routes added
}

TEST(Http1ServletTest, ConnectionTracking) {
    Http1Servlet server(8080, 4);
    
    server.get("/", [](const Request& req) {
        Response res;
        res.set_body("OK");
        return res;
    });
    
    // Multiple requests should work
    for (int i = 0; i < 5; ++i) {
        Request req;
        req.set_path("/");
        Response res = server.handle_request(req);
        EXPECT_EQ(res.status(), HttpStatus::OK);
    }
}

// ============================================================================
// HTTP/2 Server Tests
// ============================================================================

TEST(Http2ServerTest, ServerConfiguration) {
    Http2Server server(8080, 4);
    
    server.enable_server_push(true);
    server.set_max_concurrent_streams(100);
    server.set_initial_window_size(65535);
    
    auto caps = server.capabilities();
    EXPECT_TRUE(caps.supports_multiplexing);
    EXPECT_TRUE(caps.supports_server_push);
    EXPECT_TRUE(caps.supports_header_compression);
}

TEST(Http2ServerTest, StreamHandling) {
    Http2Server server(8080, 4);
    
    server.get("/stream", [](const Request& req) {
        Response res;
        res.set_body("Stream response");
        return res;
    });
    
    Request req;
    req.set_path("/stream");
    
    Response res = server.handle_request(req);
    EXPECT_EQ(res.body(), "Stream response");
}

// ============================================================================
// HTTP/3 Server Tests
// ============================================================================

TEST(Http3ServerTest, QuicConfiguration) {
    Http3Server server(8080, 4);
    
    server.enable_0rtt(true);
    server.set_max_idle_timeout(30000);
    server.set_max_udp_payload_size(1200);
    
    auto caps = server.capabilities();
    EXPECT_TRUE(caps.is_udp_based);
    EXPECT_TRUE(caps.supports_multiplexing);
}

TEST(Http3ServerTest, RequestHandling) {
    Http3Server server(8080, 4);
    
    server.get("/quic", [](const Request& req) {
        Response res;
        res.set_body("QUIC response");
        return res;
    });
    
    Request req;
    req.set_path("/quic");
    
    Response res = server.handle_request(req);
    EXPECT_EQ(res.body(), "QUIC response");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(RestApiIntegrationTest, JsonRequestResponse) {
    Server server(8080, 4);
    
    server.post("/api/user", [](const Request& req) {
        Parser parser;
        Value v = parser.parse(req.body());
        
        Response res;
        if (v.is_object()) {
            Builder builder;
            builder.add("status", "success")
                   .add("name", v.as_object().get("name").as_string());
            res.set_json(builder.build().to_string());
        } else {
            res.set_status(HttpStatus::BAD_REQUEST);
        }
        return res;
    });
    
    Request req;
    req.set_method("POST");
    req.set_path("/api/user");
    req.set_body("{\"name\":\"Alice\"}");
    
    Response res = server.handle_request(req);
    EXPECT_EQ(res.status(), HttpStatus::OK);
    EXPECT_TRUE(res.body().find("Alice") != std::string::npos);
}

TEST(RestApiIntegrationTest, MultiProtocolServers) {
    auto http1 = HttpServerFactory::create_http1(8080, 4);
    auto http2 = HttpServerFactory::create_http2(8081, 4);
    auto http3 = HttpServerFactory::create_http3(8082, 4);
    
    // Add same route to all servers
    auto handler = [](const Request& req) {
        Response res;
        res.set_body("Hello");
        return res;
    };
    
    http1->get("/", handler);
    http2->get("/", handler);
    http3->get("/", handler);
    
    Request req;
    req.set_path("/");
    
    EXPECT_EQ(http1->handle_request(req).body(), "Hello");
    EXPECT_EQ(http2->handle_request(req).body(), "Hello");
    EXPECT_EQ(http3->handle_request(req).body(), "Hello");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
