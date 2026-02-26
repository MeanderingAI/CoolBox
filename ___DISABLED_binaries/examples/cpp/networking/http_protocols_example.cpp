/**
 * HTTP Protocol Versions Example
 * 
 * Demonstrates:
 * - HTTP/1.1 server (persistent connections, keep-alive)
 * - HTTP/2 server (multiplexing, header compression, server push)
 * - HTTP/3 server (QUIC, 0-RTT, improved performance)
 * - Protocol capabilities comparison
 * - Factory pattern for creating servers
 */

#include "networking/rest_api/http_server_base.h"
#include "networking/rest_api/http1_server.h"
#include "networking/rest_api/http2_server.h"
#include "networking/rest_api/http3_server.h"
#include "dataformats/json/json.h"
#include <iostream>

using namespace networking::rest_api;
using namespace networking::json;

void print_capabilities(const std::string& name, const ProtocolCapabilities& caps) {
    std::cout << "\n" << name << " Capabilities:" << std::endl;
    std::cout << "  Multiplexing:        " << (caps.supports_multiplexing ? "✓" : "✗") << std::endl;
    std::cout << "  Server Push:         " << (caps.supports_server_push ? "✓" : "✗") << std::endl;
    std::cout << "  Header Compression:  " << (caps.supports_header_compression ? "✓" : "✗") << std::endl;
    std::cout << "  Prioritization:      " << (caps.supports_prioritization ? "✓" : "✗") << std::endl;
    std::cout << "  Encrypted:           " << (caps.is_encrypted ? "✓" : "✗") << std::endl;
    std::cout << "  UDP-based:           " << (caps.is_udp_based ? "✓" : "✗") << std::endl;
}

void add_routes(HttpServerBase& server) {
    // GET: Hello endpoint
    server.get("/", [](const Request& req) {
        Builder json_builder;
        json_builder.add("message", "Hello from REST API!")
                   .add("protocol", "Check Server header");
        
        Response res;
        res.set_status(HttpStatus::OK);
        res.set_json(json_builder.build().to_string());
        return res;
    });
    
    // GET: Protocol info
    server.get("/protocol", [&server](const Request& req) {
        Builder json_builder;
        json_builder.add("version", server.protocol_name())
                   .add("port", std::to_string(server.port()))
                   .add("threads", std::to_string(server.num_threads()));
        
        auto caps = server.capabilities();
        Builder caps_builder;
        caps_builder.add("multiplexing", caps.supports_multiplexing ? "true" : "false")
                   .add("server_push", caps.supports_server_push ? "true" : "false")
                   .add("header_compression", caps.supports_header_compression ? "true" : "false")
                   .add("prioritization", caps.supports_prioritization ? "true" : "false")
                   .add("encrypted", caps.is_encrypted ? "true" : "false")
                   .add("udp_based", caps.is_udp_based ? "true" : "false");
        
        json_builder.add("capabilities", caps_builder.build());
        
        Response res;
        res.set_status(HttpStatus::OK);
        res.set_json(json_builder.build().to_string());
        return res;
    });
    
    // POST: Echo endpoint
    server.post("/echo", [](const Request& req) {
        Builder json_builder;
        json_builder.add("method", req.method())
                   .add("path", req.path())
                   .add("body", req.body());
        
        Response res;
        res.set_status(HttpStatus::OK);
        res.set_json(json_builder.build().to_string());
        return res;
    });
}

int main() {
    std::cout << "=== HTTP Protocol Versions Example ===" << std::endl;
    
    // ========================================
    // 1. Protocol Capabilities Comparison
    // ========================================
    std::cout << "\n1. Protocol Capabilities Comparison" << std::endl;
    std::cout << "====================================" << std::endl;
    
    auto http1_caps = ProtocolCapabilities::for_version(HttpVersion::HTTP_1_1);
    auto http2_caps = ProtocolCapabilities::for_version(HttpVersion::HTTP_2);
    auto http3_caps = ProtocolCapabilities::for_version(HttpVersion::HTTP_3);
    
    print_capabilities("HTTP/1.1", http1_caps);
    print_capabilities("HTTP/2", http2_caps);
    print_capabilities("HTTP/3", http3_caps);
    
    // ========================================
    // 2. HTTP/1.1 Server
    // ========================================
    std::cout << "\n\n2. HTTP/1.1 Server" << std::endl;
    std::cout << "==================" << std::endl;
    
    auto http1_server = HttpServerFactory::create_http1(8080, 4);
    add_routes(*http1_server);
    
    // Configure HTTP/1.1 settings
    auto* http1 = dynamic_cast<Http1Server*>(http1_server.get());
    if (http1) {
        http1->set_keep_alive(true, 5);  // 5 second timeout
    }
    
    http1_server->start();
    
    // Test request
    Request req1;
    req1.set_method("GET");
    req1.set_path("/protocol");
    
    Response res1 = http1_server->handle_request(req1);
    std::cout << "\nResponse: " << res1.body() << std::endl;
    
    http1_server->stop();
    
    // ========================================
    // 3. HTTP/2 Server
    // ========================================
    std::cout << "\n\n3. HTTP/2 Server" << std::endl;
    std::cout << "================" << std::endl;
    
    auto http2_server = HttpServerFactory::create_http2(8081, 4);
    add_routes(*http2_server);
    
    // Configure HTTP/2 settings
    auto* http2 = dynamic_cast<Http2Server*>(http2_server.get());
    if (http2) {
        http2->enable_server_push(true);
        http2->set_max_concurrent_streams(100);
        http2->set_initial_window_size(65535);
    }
    
    http2_server->start();
    
    // Test request
    Request req2;
    req2.set_method("GET");
    req2.set_path("/protocol");
    
    Response res2 = http2_server->handle_request(req2);
    std::cout << "\nResponse: " << res2.body() << std::endl;
    
    http2_server->stop();
    
    // ========================================
    // 4. HTTP/3 Server
    // ========================================
    std::cout << "\n\n4. HTTP/3 Server" << std::endl;
    std::cout << "================" << std::endl;
    
    auto http3_server = HttpServerFactory::create_http3(8082, 4);
    add_routes(*http3_server);
    
    // Configure HTTP/3 settings
    auto* http3 = dynamic_cast<Http3Server*>(http3_server.get());
    if (http3) {
        http3->enable_0rtt(true);
        http3->set_max_idle_timeout(30000);  // 30 seconds
        http3->set_max_udp_payload_size(1200);
    }
    
    http3_server->start();
    
    // Test request
    Request req3;
    req3.set_method("GET");
    req3.set_path("/protocol");
    
    Response res3 = http3_server->handle_request(req3);
    std::cout << "\nResponse: " << res3.body() << std::endl;
    
    http3_server->stop();
    
    // ========================================
    // 5. Factory Pattern Usage
    // ========================================
    std::cout << "\n\n5. Factory Pattern" << std::endl;
    std::cout << "==================" << std::endl;
    
    for (auto version : {HttpVersion::HTTP_1_1, HttpVersion::HTTP_2, HttpVersion::HTTP_3}) {
        auto server = HttpServerFactory::create(version, 9000, 4);
        
        std::cout << "\nCreated: " << server->protocol_name() 
                 << " on port " << server->port() << std::endl;
        
        server->start();
        
        // Add a simple route
        server->get("/version", [&server](const Request& req) {
            Response res;
            res.set_status(HttpStatus::OK);
            res.set_json("{\"version\": \"" + server->protocol_name() + "\"}");
            return res;
        });
        
        // Test
        Request req;
        req.set_method("GET");
        req.set_path("/version");
        Response res = server->handle_request(req);
        std::cout << "Response: " << res.body() << std::endl;
        
        server->stop();
    }
    
    // ========================================
    // 6. Performance Comparison
    // ========================================
    std::cout << "\n\n6. Performance Characteristics" << std::endl;
    std::cout << "===============================" << std::endl;
    
    std::cout << "\nHTTP/1.1:" << std::endl;
    std::cout << "  - Connection: Persistent (Keep-Alive)" << std::endl;
    std::cout << "  - Latency: Moderate (head-of-line blocking)" << std::endl;
    std::cout << "  - Best for: Simple APIs, backward compatibility" << std::endl;
    
    std::cout << "\nHTTP/2:" << std::endl;
    std::cout << "  - Connection: Multiplexed streams" << std::endl;
    std::cout << "  - Latency: Low (parallel requests)" << std::endl;
    std::cout << "  - Best for: Modern web apps, high-traffic APIs" << std::endl;
    std::cout << "  - Features: Server push, header compression (HPACK)" << std::endl;
    
    std::cout << "\nHTTP/3:" << std::endl;
    std::cout << "  - Connection: QUIC over UDP" << std::endl;
    std::cout << "  - Latency: Lowest (0-RTT, no head-of-line blocking)" << std::endl;
    std::cout << "  - Best for: Mobile networks, lossy connections" << std::endl;
    std::cout << "  - Features: Built-in encryption, improved congestion control" << std::endl;
    
    std::cout << "\n✓ Example complete!" << std::endl;
    
    return 0;
}
