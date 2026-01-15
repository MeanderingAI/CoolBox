#include "IO/servlets/httpU_servlet.h"

namespace io {
namespace servlets {

// Example implementation of the factory method
template <HttpVersion V>
class HttpUServletImpl : public HttpUServlet {
public:
    HttpVersion get_version() const override { return V; }
    void handle_request(const std::string& request, std::string& response) override {
        // Placeholder: actual logic per version
        response = "Handled by HTTP version " + std::to_string(static_cast<int>(V));
    }
};

std::unique_ptr<HttpUServlet> HttpUServlet::create(const std::string& version_str) {
    HttpVersion v = detect_http_version(version_str);
    switch (v) {
        case HttpVersion::HTTP_1:
            return std::make_unique<HttpUServletImpl<HttpVersion::HTTP_1>>();
        case HttpVersion::HTTP_2:
            return std::make_unique<HttpUServletImpl<HttpVersion::HTTP_2>>();
        case HttpVersion::HTTP_3:
            return std::make_unique<HttpUServletImpl<HttpVersion::HTTP_3>>();
        default:
            return nullptr;
    }
}

} // namespace servlets
} // namespace io
