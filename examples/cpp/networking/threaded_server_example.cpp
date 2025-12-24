/**
 * Threaded Server Example
 * 
 * Demonstrates:
 * - Thread pool configuration
 * - Asynchronous request handling
 * - Concurrent request processing
 * - Performance with multiple threads
 */

#include "networking/rest_api/server.h"
#include "dataformats/json/json.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

using namespace networking::rest_api;
using namespace networking::json;

// Simulated expensive computation
void expensive_computation(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
    std::cout << "=== Threaded Server Example ===" << std::endl;
    
    // Create server with 8 threads for better concurrency
    Server server(8080, 8);
    
    // Add a route that simulates expensive computation
    server.get("/compute/:duration", [](const Request& req) {
        auto params = req.path_params();
        int duration = 100;  // default
        
        auto it = params.find("duration");
        if (it != params.end()) {
            try {
                duration = std::stoi(it->second);
            } catch (...) {
                duration = 100;
            }
        }
        
        // Simulate expensive computation
        expensive_computation(duration);
        
        Builder json_builder;
        json_builder.add("computation_time_ms", duration)
                   .add("message", "Computation complete")
                   .add("thread_id", std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
        
        Response res;
        res.set_status(HttpStatus::OK);
        res.set_json(json_builder.build().to_string());
        return res;
    });
    
    // Add a quick response route
    server.get("/ping", [](const Request& req) {
        Builder json_builder;
        json_builder.add("status", "ok")
                   .add("timestamp", std::chrono::system_clock::now().time_since_epoch().count());
        
        Response res;
        res.set_status(HttpStatus::OK);
        res.set_json(json_builder.build().to_string());
        return res;
    });
    
    server.start();
    
    // ========================================
    // Test 1: Synchronous requests
    // ========================================
    std::cout << "\nTest 1: Synchronous Requests (blocking)" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    Request req1;
    req1.set_method("GET");
    req1.set_path("/compute/50");
    
    Request req2;
    req2.set_method("GET");
    req2.set_path("/compute/50");
    
    Response res1 = server.handle_request(req1);
    Response res2 = server.handle_request(req2);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Request 1: " << res1.body() << std::endl;
    std::cout << "Request 2: " << res2.body() << std::endl;
    std::cout << "Total time (sequential): " << duration.count() << "ms" << std::endl;
    
    // ========================================
    // Test 2: Asynchronous requests
    // ========================================
    std::cout << "\nTest 2: Asynchronous Requests (concurrent)" << std::endl;
    
    std::atomic<int> completed{0};
    
    start = std::chrono::high_resolution_clock::now();
    
    // Submit multiple async requests
    server.handle_request_async(req1, [&completed](const Response& res) {
        std::cout << "Async response 1: " << res.body() << std::endl;
        completed++;
    });
    
    server.handle_request_async(req2, [&completed](const Response& res) {
        std::cout << "Async response 2: " << res.body() << std::endl;
        completed++;
    });
    
    Request req3;
    req3.set_method("GET");
    req3.set_path("/compute/50");
    
    server.handle_request_async(req3, [&completed](const Response& res) {
        std::cout << "Async response 3: " << res.body() << std::endl;
        completed++;
    });
    
    // Wait for all async requests to complete
    while (completed < 3) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Total time (concurrent): " << duration.count() << "ms" << std::endl;
    std::cout << "(Should be ~50ms with parallelism vs ~150ms sequential)" << std::endl;
    
    // ========================================
    // Test 3: High volume requests
    // ========================================
    std::cout << "\nTest 3: High Volume Requests" << std::endl;
    
    const int num_requests = 20;
    std::atomic<int> high_volume_completed{0};
    
    Request ping_req;
    ping_req.set_method("GET");
    ping_req.set_path("/ping");
    
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_requests; ++i) {
        server.handle_request_async(ping_req, [&high_volume_completed](const Response& res) {
            high_volume_completed++;
        });
    }
    
    // Wait for all requests
    while (high_volume_completed < num_requests) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Processed " << num_requests << " requests in " << duration.count() << "ms" << std::endl;
    std::cout << "Average: " << (duration.count() / (double)num_requests) << "ms per request" << std::endl;
    
    server.stop();
    return 0;
}
