#pragma once
#include <string>
#include <memory>
#include "networking/http/request_response.h"

namespace networking {
namespace rest_api {


// Abstract base handler for all HTTP versions
class StubbedHandler {
public:
    virtual ~StubbedHandler() = default;
    // Must be implemented: handle a request and return a response
    virtual http::Response handle(const http::Request& request) = 0;
};

// File serving handler
class FileHandler : public StubbedHandler {
public:
    // root_dir: directory to serve files from (e.g., "static/")
    explicit FileHandler(const std::string& root_dir);
    http::Response handle(const http::Request& request) override;
private:
    std::string root_dir_;
    // Utility: guess content type from file extension
    static std::string get_mime_type(const std::string& filename);
};

} // namespace rest_api
} // namespace networking
