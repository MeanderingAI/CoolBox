#include "gtest/gtest.h"
#include "IO/servlets/http1_servlet.h"
#include "IO/servlets/http2_servlet.h"
#include "IO/servlets/http3_servlet.h"
#include "IO/servlets/httpU_servlet.h"

using namespace networking::servlets;
using namespace io::servlets;

// Dummy request/response for testing
namespace http = ::http;

class DummyRequest : public http::Request {
public:
    DummyRequest() { method = "GET"; uri = "/"; }
};

class DummyServlet : public HttpServletBase {
public:
    http::Response handle_request(const http::Request& request) override {
        http::Response resp;
        resp.status_code = 200;
        resp.body = "ok";
        return resp;
    }
};

TEST(Http1ServletTest, CanInstantiateAndHandle) {
    Http1Servlet servlet;
    DummyRequest req;
    auto resp = servlet.handle_request(req);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_EQ(resp.body, "ok");
}

TEST(Http2ServletTest, CanInstantiateAndHandle) {
    Http2Servlet servlet;
    DummyRequest req;
    auto resp = servlet.handle_request(req);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_EQ(resp.body, "ok");
}

TEST(Http3ServletTest, CanInstantiateAndHandle) {
    Http3Servlet servlet;
    DummyRequest req;
    auto resp = servlet.handle_request(req);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_EQ(resp.body, "ok");
}

// Test the universal servlet factory and dispatch
TEST(HttpUServletTest, FactoryCreatesCorrectVersion) {
    auto s1 = HttpUServlet::create("HTTP/1");
    EXPECT_TRUE(s1);
    EXPECT_EQ(s1->get_version(), HttpVersion::HTTP_1);
    auto s2 = HttpUServlet::create("HTTP/2");
    EXPECT_TRUE(s2);
    EXPECT_EQ(s2->get_version(), HttpVersion::HTTP_2);
    auto s3 = HttpUServlet::create("HTTP/3");
    EXPECT_TRUE(s3);
    EXPECT_EQ(s3->get_version(), HttpVersion::HTTP_3);
}

// Test the universal servlet's request handling
TEST(HttpUServletTest, HandlesRequest) {
    auto servlet = HttpUServlet::create("HTTP/1");
    std::string req = "GET /";
    std::string resp;
    servlet->handle_request(req, resp);
    EXPECT_FALSE(resp.empty());
}
