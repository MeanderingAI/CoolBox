#include "gtest/gtest.h"
#include "IO/servlets/http_server.h"
#include "IO/servlets/HttpVersion.h"

using namespace io::servlets;

TEST(HttpServerTest, VersionIsSetCorrectly) {
    HttpServer server(HttpVersion::HTTP_2, 4);
    EXPECT_EQ(server.get_version(), HttpVersion::HTTP_2);
}

TEST(HttpServerTest, ThreadPoolCreated) {
    HttpServer server(HttpVersion::HTTP_1, 2);
    EXPECT_EQ(server.get_version(), HttpVersion::HTTP_1);
    // Additional checks for thread pool can be added if interface allows
}

// Add more tests for start/stop logic as implemented
