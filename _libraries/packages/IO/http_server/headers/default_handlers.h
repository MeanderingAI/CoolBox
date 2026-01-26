
// Only one set of class and namespace definitions, properly closed
#pragma once
#include <string>
#include <fstream>
#include "request_handle.h"
#include "request_response.h"

namespace io {
namespace http_server {

class DefaultHandler {
public:
    virtual ~DefaultHandler() = default;
    virtual Response handle(const std::string& path) = 0;
};

class FileHandler : public DefaultHandler {
public:
    explicit FileHandler(const std::string& base_path);
    Response handle(const std::string& rel_path) override;
private:
    std::string base_path_;
    static std::string get_mime_type(const std::string& path);
};

// Inline implementations for header-only usage
inline FileHandler::FileHandler(const std::string& base_path) : base_path_(base_path) {}

inline Response FileHandler::handle(const std::string& rel_path) {
    std::string full_path = base_path_ + "/" + rel_path;
    std::ifstream file(full_path, std::ios::binary);
    if (!file) {
        return Response::not_found();
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    Response resp = Response::ok(content);
    // Set Content-Type header based on file extension
    std::string mime = get_mime_type(full_path);
    resp.headers[HeaderKey::ContentType] = mime;
    return resp;
}

inline std::string FileHandler::get_mime_type(const std::string& path) {
    auto ext_pos = path.find_last_of('.');
    if (ext_pos == std::string::npos) return "application/octet-stream";
    std::string ext = path.substr(ext_pos + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js" || ext == "mjs") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "ico") return "image/x-icon";
    if (ext == "txt") return "text/plain";
    if (ext == "wasm") return "application/wasm";
    if (ext == "pdf") return "application/pdf";
    if (ext == "csv") return "text/csv";
    // Add more as needed
    return "application/octet-stream";
}

} // namespace http_server
} // namespace io

