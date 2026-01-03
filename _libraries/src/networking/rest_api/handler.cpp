#include "networking/rest_api/handler.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace networking {
namespace rest_api {


FileHandler::FileHandler(const std::string& root_dir)
    : root_dir_(root_dir) {}

http::Response FileHandler::handle(const http::Request& request) {
    // Extract file path from request (e.g., /static/fetch_logger.js)
    std::string rel_path = request.path();
    if (rel_path.find("/static/") == 0) {
        rel_path = rel_path.substr(8); // remove "/static/"
    }
    std::string file_path = root_dir_ + "/" + rel_path;
    if (!std::filesystem::exists(file_path)) {
        http::Response resp(404, "File not found");
        resp.set_header("Content-Type", "text/plain");
        return resp;
    }
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        http::Response resp(500, "Failed to open file");
        resp.set_header("Content-Type", "text/plain");
        return resp;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string body = ss.str();
    http::Response resp(200, body);
    resp.set_header("Content-Type", get_mime_type(file_path));
    return resp;
}

std::string FileHandler::get_mime_type(const std::string& filename) {
    auto ext = std::filesystem::path(filename).extension().string();
    if (ext == ".js") return "application/javascript";
    if (ext == ".css") return "text/css";
    if (ext == ".html") return "text/html";
    if (ext == ".json") return "application/json";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    return "application/octet-stream";
}

} // namespace rest_api
} // namespace networking
