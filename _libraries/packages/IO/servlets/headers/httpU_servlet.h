#pragma once

#include <string>
#include <memory>

namespace io {
namespace servlets {

// Enum for HTTP version
enum class HttpVersion {
    HTTP_1,
    HTTP_2,
    HTTP_3,
    UNKNOWN
};

// Utility to convert string to HttpVersion
inline HttpVersion detect_http_version(const std::string& version_str) {
    if (version_str.find("2") != std::string::npos) return HttpVersion::HTTP_2;
    if (version_str.find("3") != std::string::npos) return HttpVersion::HTTP_3;
    if (version_str.find("1") != std::string::npos) return HttpVersion::HTTP_1;
    return HttpVersion::UNKNOWN;
}

// Example base class for a universal HTTP servlet
class HttpUServlet {
public:
    virtual ~HttpUServlet() = default;
    virtual HttpVersion get_version() const = 0;
    virtual void handle_request(const std::string& request, std::string& response) = 0;

    // Factory to create the correct servlet based on version string
    static std::unique_ptr<HttpUServlet> create(const std::string& version_str);
};

} // namespace servlets
} // namespace io
