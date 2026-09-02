#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>

#include "http_server.h"
#include "http_servlet_base.h"
#include "interpreter_php.h"
#include "lexer_php.h"
#include "parser_php.h"

namespace {

std::string path_without_query(const std::string& uri) {
    const auto pos = uri.find('?');
    return pos == std::string::npos ? uri : uri.substr(0, pos);
}

std::string url_decode(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            decoded.push_back(' ');
            continue;
        }
        if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            const long code = std::strtol(hex.c_str(), &end, 16);
            if (end && *end == '\0') {
                decoded.push_back(static_cast<char>(code));
                i += 2;
                continue;
            }
        }
        decoded.push_back(value[i]);
    }
    return decoded;
}

std::map<std::string, std::string> parse_query_string(const std::string& query) {
    std::map<std::string, std::string> params;
    std::istringstream iss(query);
    std::string token;
    while (std::getline(iss, token, '&')) {
        if (token.empty()) continue;
        const auto eq = token.find('=');
        if (eq == std::string::npos) {
            params[url_decode(token)] = "";
        } else {
            params[url_decode(token.substr(0, eq))] = url_decode(token.substr(eq + 1));
        }
    }
    return params;
}

std::map<std::string, std::string> parse_get_params(const std::string& uri) {
    const auto pos = uri.find('?');
    if (pos == std::string::npos || pos + 1 >= uri.size()) return {};
    return parse_query_string(uri.substr(pos + 1));
}

std::string guess_content_type(const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return "application/octet-stream";
    const std::string ext = path.substr(dot + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "svg") return "image/svg+xml";
    return "application/octet-stream";
}

bool has_suffix(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Response make_response(int status, const std::string& body, const std::string& content_type) {
    Response r;
    r.status_code = status;
    r.body = body;
    r.headers[HeaderKey::ContentType] = content_type;
    r.headers[HeaderKey::ContentLength] = std::to_string(body.size());
    return r;
}

// Serves .php files under a webroot directory by lexing/parsing/interpreting them
// and returning the resulting HTML through the existing HTTP server/servlet stack.
class PhpServlet : public networking::servlets::HttpServletBase {
public:
    explicit PhpServlet(std::string webroot) : webroot_(std::move(webroot)) {}

    std::string get_version() const override { return "HTTP/1.1"; }

    Response handle_request(const Request& req) override {
        std::string path = path_without_query(req.uri);
        if (path.empty() || path == "/") path = "/index.php";

        // Prevent path traversal outside the configured webroot.
        if (path.find("..") != std::string::npos) {
            return make_response(400, "Bad request", "text/plain");
        }

        const std::string file_path = webroot_ + path;
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return make_response(404, "404 Not Found", "text/plain");
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        const std::string source = contents.str();

        if (!has_suffix(path, ".php")) {
            return make_response(200, source, guess_content_type(path));
        }

        try {
            plphp::LexerPHP lexer(source);
            plphp::ParserPHP parser(lexer.tokenize());
            auto program = parser.parse_program();

            plphp::InterpreterPHP interpreter;
            interpreter.set_get_params(parse_get_params(req.uri));
            interpreter.set_post_params(parse_query_string(req.body));
            interpreter.set_server_params({
                {"REQUEST_METHOD", req.method},
                {"REQUEST_URI", req.uri},
                {"SCRIPT_NAME", path},
            });

            const std::string html = interpreter.run(program);
            return make_response(200, html, "text/html");
        } catch (const std::exception& ex) {
            std::ostringstream body;
            body << "<h1>500 Internal Server Error</h1><pre>" << ex.what() << "</pre>";
            return make_response(500, body.str(), "text/html");
        }
    }

private:
    std::string webroot_;
};

std::atomic<int> stop_flag{0};

void signal_handler(int) {
    stop_flag = 1;
}

} // namespace

int main(int argc, char** argv) {
    int port = 8090;
    std::string webroot = "_deliverables/apps/php_center_api/webroot";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else if (arg == "--webroot" && i + 1 < argc) {
            webroot = argv[++i];
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto servlet = std::make_shared<PhpServlet>(webroot);
    io::http_server::HttpServer server(port, 4, nullptr, servlet);

    std::thread server_thread([&server]() { server.start(); });

    std::cout << "[php_center_api] serving '" << webroot << "' on port " << port << std::endl;

    while (!stop_flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[php_center_api] shutting down" << std::endl;
    server.stop();
    if (server_thread.joinable()) server_thread.join();

    return 0;
}
