
#include "http_server.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <unistd.h>
#endif


namespace io {
namespace http_server {

namespace {

std::string json_escape(const std::string& value) {
    std::ostringstream escaped;
    for (unsigned char ch : value) {
        switch (ch) {
            case '\\': escaped << "\\\\"; break;
            case '"': escaped << "\\\""; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default: escaped << static_cast<char>(ch); break;
        }
    }
    return escaped.str();
}

std::string json_unescape(const std::string& value) {
    std::string output;
    output.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            output.push_back(value[i]);
            continue;
        }
        const char escaped = value[++i];
        switch (escaped) {
            case '\\': output.push_back('\\'); break;
            case '"': output.push_back('"'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            default: output.push_back(escaped); break;
        }
    }
    return output;
}

std::string extract_json_string_field(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"((?:\\\\.|[^\\\"])*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return "";
    }
    return json_unescape(match[1].str());
}

std::string get_header_value(const Request& request, HeaderKey header_key) {
    for (const auto& [key, value] : request.headers) {
        if (std::holds_alternative<HeaderKey>(key) && std::get<HeaderKey>(key) == header_key) {
            return value;
        }
    }
    return "";
}

Response json_response(int status_code, const std::string& body) {
    Response response;
    response.status_code = status_code;
    response.body = body;
    response.headers[HeaderKey::ContentType] = "application/json";
    response.headers[HeaderKey::ContentLength] = std::to_string(body.size());
    return response;
}

Response auth_error_response(int status_code, const std::string& message) {
    Response response = json_response(status_code,
        std::string("{\"success\":false,\"message\":\"") + json_escape(message) + "\"}");
    if (status_code == 401) {
        response.headers[std::string("WWW-Authenticate")] = "Bearer";
    }
    return response;
}

bool path_matches_prefix(const std::string& uri, const std::string& prefix) {
    if (prefix.empty()) {
        return false;
    }
    if (uri == prefix) {
        return true;
    }
    if (uri.rfind(prefix, 0) != 0 || uri.size() <= prefix.size()) {
        return false;
    }
    return prefix.back() == '/' || uri[prefix.size()] == '/' || uri[prefix.size()] == '?';
}

} // namespace


HttpServer::HttpServer(int port, size_t num_threads, advanced_logging::Logger* logger, std::shared_ptr<networking::servlets::HttpServletBase> servlet)
    : port_(port), num_threads_(num_threads), logger_(logger), servlet_(std::move(servlet)), thread_pool_(std::make_unique<ThreadPool>(num_threads)) {}


HttpServer::~HttpServer() {
    stop();
}


void HttpServer::start() {
    std::cout << "[HttpServer] Starting server on port " << port_ << ", version: " << get_version() << std::endl;
    running_ = true;

    // Initialize Winsock on Windows
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "[HttpServer] WSAStartup failed." << std::endl;
        return;
    }
#endif

#ifdef _WIN32
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "[HttpServer] Failed to create socket." << std::endl;
        WSACleanup();
        return;
    }
#else
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[HttpServer] Failed to create socket." << std::endl;
        return;
    }
#endif

    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), static_cast<int>(sizeof(opt)));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "[HttpServer] Failed to bind socket." << std::endl;
#ifdef _WIN32
            closesocket(server_fd);
            WSACleanup();
#else
            close(server_fd);
#endif
            return;
        }

        if (listen(server_fd, 10) < 0) {
            std::cerr << "[HttpServer] Failed to listen on socket." << std::endl;
#ifdef _WIN32
            closesocket(server_fd);
            WSACleanup();
#else
            close(server_fd);
#endif
            return;
        }

    std::cout << "[HttpServer] Listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in client_addr;
#ifdef _WIN32
        int client_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == INVALID_SOCKET) {
            if (running_) {
                std::cerr << "[HttpServer] Accept failed." << std::endl;
            }
            continue;
        }

        thread_pool_->enqueue([this, client_fd]() {
#else
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "[HttpServer] Accept failed." << std::endl;
            }
            continue;
        }

        thread_pool_->enqueue([this, client_fd]() {
#endif
            char buffer[4096];
#ifdef _WIN32
            int bytes_read = recv(client_fd, buffer, static_cast<int>(sizeof(buffer) - 1), 0);
#else
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
#endif
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                Request req = Request::from_string(std::string(buffer));
                Response resp = handle_request_with_auth(req);
                std::string resp_str = resp.to_string();
#ifdef _WIN32
                send(client_fd, resp_str.c_str(), static_cast<int>(resp_str.size()), 0);
                closesocket(client_fd);
#else
                write(client_fd, resp_str.c_str(), resp_str.size());
                close(client_fd);
#endif
            }
        });
    }

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
}

void HttpServer::set_auth_system(std::shared_ptr<auth::AuthSystem> auth_system) {
    auth_system_ = std::move(auth_system);
}

void HttpServer::protect_path_prefix(const std::string& path_prefix, auth::UserRole required_role) {
    protected_paths_.push_back({path_prefix, required_role});
}

void HttpServer::clear_protected_paths() {
    protected_paths_.clear();
}

void HttpServer::set_auth_endpoints(const std::string& login_path,
                                    const std::string& logout_path,
                                    const std::string& me_path) {
    login_endpoint_ = login_path;
    logout_endpoint_ = logout_path;
    me_endpoint_ = me_path;
}

Response HttpServer::handle_request_with_auth(const Request& request) {
    if (!servlet_) {
        return auth_error_response(500, "HTTP servlet is not configured");
    }

    if (!auth_system_) {
        return servlet_->handle_request(request);
    }

    if (request.uri == login_endpoint_) {
        if (request.method != "POST") {
            return auth_error_response(405, "Login endpoint only accepts POST requests");
        }

        const std::string username = extract_json_string_field(request.body, "username");
        const std::string password = extract_json_string_field(request.body, "password");
        if (username.empty() || password.empty()) {
            return auth_error_response(400, "Request body must contain JSON username and password fields");
        }

        const auth::AuthResult result = auth_system_->login(username, password);
        if (!result.success) {
            return auth_error_response(401, result.message);
        }

        std::ostringstream body;
        body << '{'
             << "\"success\":true,"
             << "\"message\":\"" << json_escape(result.message) << "\","
             << "\"session_id\":\"" << json_escape(result.session_id) << "\","
             << "\"token\":\"" << json_escape(result.jwt_token) << "\","
             << "\"token_type\":\"Bearer\","
             << "\"expires_in\":" << auth_system_->get_jwt_lifetime() << ','
             << "\"user\":{"
             << "\"username\":\"" << json_escape(result.user.username) << "\","
             << "\"email\":\"" << json_escape(result.user.email) << "\","
             << "\"role\":\"" << json_escape(auth::role_to_string(result.user.role)) << "\""
             << "}}";
        return json_response(200, body.str());
    }

    if (request.uri == logout_endpoint_) {
        if (request.method != "POST") {
            return auth_error_response(405, "Logout endpoint only accepts POST requests");
        }

        const auto validation = auth_system_->validate_authorization_header(get_header_value(request, HeaderKey::Authorization));
        if (!validation.valid) {
            return auth_error_response(401, validation.message);
        }

        auth_system_->logout(validation.claims.session_id);
        return json_response(200, "{\"success\":true,\"message\":\"Logout successful\"}");
    }

    if (request.uri == me_endpoint_) {
        if (request.method != "GET") {
            return auth_error_response(405, "Profile endpoint only accepts GET requests");
        }

        const auto validation = auth_system_->validate_authorization_header(get_header_value(request, HeaderKey::Authorization));
        if (!validation.valid) {
            return auth_error_response(401, validation.message);
        }

        std::ostringstream body;
        body << '{'
             << "\"authenticated\":true,"
             << "\"username\":\"" << json_escape(validation.claims.subject) << "\","
             << "\"role\":\"" << json_escape(auth::role_to_string(validation.claims.role)) << "\","
             << "\"session_id\":\"" << json_escape(validation.claims.session_id) << "\","
             << "\"issuer\":\"" << json_escape(validation.claims.issuer) << "\","
             << "\"issued_at\":" << validation.claims.issued_at << ','
             << "\"expires_at\":" << validation.claims.expires_at
             << '}';
        return json_response(200, body.str());
    }

    auth::UserRole required_role = auth::UserRole::USER;
    bool requires_auth = false;
    for (const auto& rule : protected_paths_) {
        if (path_matches_prefix(request.uri, rule.path_prefix)) {
            required_role = rule.required_role;
            requires_auth = true;
            break;
        }
    }

    if (!requires_auth) {
        return servlet_->handle_request(request);
    }

    const auto validation = auth_system_->validate_authorization_header(get_header_value(request, HeaderKey::Authorization));
    if (!validation.valid) {
        return auth_error_response(401, validation.message);
    }

    if (static_cast<int>(validation.claims.role) > static_cast<int>(required_role)) {
        return auth_error_response(403, "JWT does not grant access to this resource");
    }

    Request authenticated_request = request;
    authenticated_request.headers[std::string("X-Authenticated-User")] = validation.claims.subject;
    authenticated_request.headers[std::string("X-Authenticated-Role")] = auth::role_to_string(validation.claims.role);
    authenticated_request.headers[std::string("X-Authenticated-Session")] = validation.claims.session_id;
    authenticated_request.headers[std::string("X-Authenticated-Issuer")] = validation.claims.issuer;
    return servlet_->handle_request(authenticated_request);
}



// Add stub for handler registration to resolve linker errors
void HttpServer::add_request_handler(const RequestHandle&) {}


void HttpServer::stop() {
    running_ = false;
}


void HttpServer::display_banner() const {
    std::cout << "[HttpServer] Banner: Service running on port " << port_ << ", version: " << get_version() << std::endl;
}


std::string HttpServer::get_version() const {
    return servlet_ ? servlet_->get_version() : "UNKNOWN";
}


} // namespace http_server
} // namespace io
