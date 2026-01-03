#include <gtest/gtest.h>
#include "networking/rest_api/http1_servlet.h"
#include "networking/rest_api/http2_server.h"
#include "networking/rest_api/http3_server.h"
#include "networking/http/request_response.h"
#include <thread>
#include <chrono>

using namespace networking::rest_api;
using namespace networking::http;

// Dummy handler for testing
Response dummy_handler(const Request& req) {
    Response res;
    res.set_status(200);
    res.set_body("Hello, World!");
    return res;
}

TEST(Http1ServletTest, StartStop) {
    Http1Servlet server(9081, 2);
    server.load_routes({std::make_shared<Route>("/", networking::http::HttpMethod::GET, dummy_handler)});
    std::thread t([&](){ server.start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    server.stop();
    t.join();
    SUCCEED();
}

TEST(Http2ServerTest, StartStop) {
    Http2Server server(9082, 2);
    server.load_routes({std::make_shared<Route>("/", networking::http::HttpMethod::GET, dummy_handler)});
    std::thread t([&](){ server.start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    server.stop();
    t.join();
    SUCCEED();
}

TEST(Http3ServerTest, StartStop) {
    Http3Server server(9083, 2);
    server.load_routes({std::make_shared<Route>("/", networking::http::HttpMethod::GET, dummy_handler)});
    std::thread t([&](){ server.start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    server.stop();
    t.join();
    SUCCEED();
}
