#include "rest_api/server.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <thread>
#include <vector>
#include <iostream>
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef DELETE
#undef DELETE
#endif
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace ml {
namespace rest_api {
namespace {

#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket invalid_socket = INVALID_SOCKET;
void close_socket(Socket socket) { closesocket(socket); }
int socket_error() { return WSAGetLastError(); }
#else
using Socket = int;
constexpr Socket invalid_socket = -1;
void close_socket(Socket socket) { close(socket); }
int socket_error() { return errno; }
#endif

constexpr std::size_t max_request_bytes = 16 * 1024 * 1024;

bool read_request(Socket client, std::string& raw) {
    char buffer[8192];
    std::size_t expected_size = 0;
    while (raw.size() < max_request_bytes) {
#ifdef _WIN32
        const int count = recv(client, buffer, sizeof(buffer), 0);
#else
        const ssize_t count = recv(client, buffer, sizeof(buffer), 0);
#endif
        if (count <= 0) return false;
        raw.append(buffer, static_cast<std::size_t>(count));
        const auto header_end = raw.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            if (expected_size == 0) {
                expected_size = header_end + 4;
                std::size_t line_begin = raw.find("\r\n") + 2;
                while (line_begin < header_end) {
                    const auto line_end = raw.find("\r\n", line_begin);
                    if (line_end == std::string::npos || line_end > header_end) break;
                    const auto colon = raw.find(':', line_begin);
                    if (colon != std::string::npos && colon < line_end) {
                        std::string name = raw.substr(line_begin, colon - line_begin);
                        std::transform(name.begin(), name.end(), name.begin(),
                                       [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
                        if (name == "content-length") {
                            const auto value_begin = raw.find_first_not_of(" \t", colon + 1);
                            if (value_begin == std::string::npos || value_begin >= line_end) return false;
                            try {
                                const auto body_size = std::stoull(raw.substr(value_begin, line_end - value_begin));
                                if (body_size > max_request_bytes - expected_size) return false;
                                expected_size += static_cast<std::size_t>(body_size);
                            } catch (const std::exception&) {
                                return false;
                            }
                            break;
                        }
                    }
                    line_begin = line_end + 2;
                }
            }
            if (raw.size() >= expected_size) return true;
        }
    }
    return false;
}

std::string path_only(const std::string& uri) {
    const auto query = uri.find('?');
    return uri.substr(0, query);
}

std::string decode_query_component(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+' ) {
            decoded.push_back(' ');
        } else if (value[i] == '%' && i + 2 < value.size()) {
            try {
                decoded.push_back(static_cast<char>(std::stoul(value.substr(i + 1, 2), nullptr, 16)));
                i += 2;
            } catch (const std::exception&) {
                decoded.push_back(value[i]);
            }
        } else {
            decoded.push_back(value[i]);
        }
    }
    return decoded;
}

std::map<std::string, std::string> parse_query_params(const std::string& uri) {
    std::map<std::string, std::string> params;
    const auto query_start = uri.find('?');
    if (query_start == std::string::npos) return params;
    std::size_t begin = query_start + 1;
    while (begin <= uri.size()) {
        const auto end = uri.find('&', begin);
        const auto pair = uri.substr(begin, end == std::string::npos ? end : end - begin);
        const auto equals = pair.find('=');
        params[decode_query_component(pair.substr(0, equals))] =
            equals == std::string::npos ? "" : decode_query_component(pair.substr(equals + 1));
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return params;
}

std::string method_name(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::OPTIONS: return "OPTIONS";
    }
    return "";
}

HttpMethod parse_method(const std::string& method) {
    if (method == "GET") return HttpMethod::GET;
    if (method == "POST") return HttpMethod::POST;
    if (method == "PUT") return HttpMethod::PUT;
    if (method == "DELETE") return HttpMethod::DELETE;
    if (method == "PATCH") return HttpMethod::PATCH;
    if (method == "OPTIONS") return HttpMethod::OPTIONS;
    throw std::runtime_error("unsupported route method: " + method);
}

int route_specificity(const std::string& pattern) {
    int specificity = 0;
    std::size_t begin = 0;
    while (begin < pattern.size()) {
        const auto end = pattern.find('/', begin);
        const auto segment = pattern.substr(
            begin, end == std::string::npos ? pattern.size() - begin : end - begin);
        if (segment.empty() || segment[0] != ':') ++specificity;
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return specificity;
}

bool match_route(const std::string& pattern, const std::string& path,
                 std::map<std::string, std::string>& params) {
    std::size_t pattern_pos = 0;
    std::size_t path_pos = 0;
    while (pattern_pos < pattern.size() && path_pos < path.size()) {
        const auto pattern_end = pattern.find('/', pattern_pos);
        const auto path_end = path.find('/', path_pos);
        const auto pattern_segment = pattern.substr(
            pattern_pos, pattern_end == std::string::npos ? pattern.size() - pattern_pos
                                                          : pattern_end - pattern_pos);
        const auto path_segment = path.substr(
            path_pos, path_end == std::string::npos ? path.size() - path_pos
                                                    : path_end - path_pos);
        if (!pattern_segment.empty() && pattern_segment[0] == ':') {
            params[pattern_segment.substr(1)] = path_segment;
        } else if (pattern_segment != path_segment) {
            return false;
        }
        if (pattern_end == std::string::npos || path_end == std::string::npos) {
            return pattern_end == path_end;
        }
        pattern_pos = pattern_end + 1;
        path_pos = path_end + 1;
    }
    return pattern_pos == pattern.size() && path_pos == path.size();
}

Response serve_client(Socket client, Server& server) {
    std::string raw;
    if (!read_request(client, raw)) {
        return Response(400, "Bad Request");
    }

    const auto first_space = raw.find(' ');
    const auto second_space = first_space == std::string::npos
        ? std::string::npos : raw.find(' ', first_space + 1);
    if (first_space == std::string::npos || second_space == std::string::npos) {
        return Response(400, "Bad Request");
    }
    const std::string method = raw.substr(0, first_space);
    HttpMethod http_method;
    if (method == "GET") http_method = HttpMethod::GET;
    else if (method == "POST") http_method = HttpMethod::POST;
    else if (method == "PUT") http_method = HttpMethod::PUT;
    else if (method == "DELETE") http_method = HttpMethod::DELETE;
    else if (method == "PATCH") http_method = HttpMethod::PATCH;
    else if (method == "OPTIONS") http_method = HttpMethod::OPTIONS;
    else return Response(405, "Method Not Allowed");

    const std::string uri = raw.substr(first_space + 1, second_space - first_space - 1);
    const auto header_end = raw.find("\r\n\r\n");
    std::map<std::string, std::string> headers;
    std::size_t line_begin = raw.find("\r\n") + 2;
    while (line_begin < header_end) {
        const auto line_end = raw.find("\r\n", line_begin);
        if (line_end == std::string::npos || line_end > header_end) break;
        const auto colon = raw.find(':', line_begin);
        if (colon != std::string::npos && colon < line_end) {
            const auto value_begin = raw.find_first_not_of(" \t", colon + 1);
            const auto value_end = raw.find_last_not_of(" \t", line_end - 1);
            headers[raw.substr(line_begin, colon - line_begin)] =
                value_begin == std::string::npos || value_begin > value_end
                    ? "" : raw.substr(value_begin, value_end - value_begin + 1);
        }
        line_begin = line_end + 2;
    }
    const std::string body = header_end == std::string::npos
        ? "" : raw.substr(header_end + 4);
    Request request(http_method, uri, headers, body);
    return server.handle_request(request);
}

void write_response(Socket client, const Response& response) {
    std::string serialized = response.to_string();
    const auto header_end = serialized.find("\r\n\r\n");
    if (header_end != std::string::npos) {
        std::string extra_headers;
        if (response.headers().find("Content-Length") == response.headers().end()) {
            extra_headers += "Content-Length: " + std::to_string(response.body().size()) + "\r\n";
        }
        extra_headers += "Connection: close\r\n";
        serialized.insert(header_end + 2, extra_headers);
    }
#ifdef _WIN32
    std::size_t sent = 0;
    while (sent < serialized.size()) {
        const int count = send(client, serialized.data() + sent,
                               static_cast<int>(serialized.size() - sent), 0);
        if (count <= 0) break;
        sent += static_cast<std::size_t>(count);
    }
#else
    std::size_t sent = 0;
    while (sent < serialized.size()) {
        const auto count = send(client, serialized.data() + sent, serialized.size() - sent,
#ifdef MSG_NOSIGNAL
                                MSG_NOSIGNAL
#else
                                0
#endif
        );
        if (count <= 0) break;
        sent += static_cast<std::size_t>(count);
    }
#endif
}

} // namespace

Response Server::handle_request(const Request& request) {
    const std::string path = path_only(request.path());
    RouteHandler matched_handler;
    std::map<std::string, std::string> path_params;
    std::string cors_origin;
    {
        std::lock_guard<std::mutex> lock(routes_mutex_);
        cors_origin = cors_origin_;
        int best_specificity = -1;
        for (const auto& [key, handler] : routes_) {
            if (key.method != request.method()) continue;
            std::map<std::string, std::string> candidate_params;
            if (match_route(key.path, path, candidate_params) &&
                route_specificity(key.path) > best_specificity) {
                matched_handler = handler;
                path_params = std::move(candidate_params);
                best_specificity = route_specificity(key.path);
            }
        }
    }
    if (!matched_handler) return Response(404, "Not Found");

    Request routed_request = request;
    routed_request.path_params_ = std::move(path_params);
    routed_request.query_params_ = parse_query_params(request.path());
    Response response = matched_handler(routed_request);
    if (!cors_origin.empty()) {
        response.set_header("Access-Control-Allow-Origin", cors_origin);
    }
    return response;
}

void Server::register_handler(const std::string& name, RouteHandler handler) {
    if (name.empty() || !handler) {
        throw std::invalid_argument("route handler name and function are required");
    }
    std::lock_guard<std::mutex> lock(routes_mutex_);
    named_handlers_[name] = std::move(handler);
}

void Server::load_routes(const std::string& routes_file) {
    std::ifstream input(routes_file);
    if (!input) {
        throw std::runtime_error("could not open routes file: " + routes_file);
    }

    struct RouteDeclaration {
        HttpMethod method;
        std::string path;
        std::string handler_name;
        std::size_t line;
    };
    std::vector<RouteDeclaration> declarations;
    std::set<std::pair<int, std::string>> seen_routes;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        std::istringstream fields(line);
        std::string method, path, handler, extra;
        if (!(fields >> method)) continue;
        if (!(fields >> path >> handler) || (fields >> extra)) {
            throw std::runtime_error("invalid route declaration at " + routes_file + ":" +
                                     std::to_string(line_number) +
                                     "; expected METHOD /path handler.name");
        }
        const auto parsed_method = parse_method(method);
        if (path.empty() || path[0] != '/') {
            throw std::runtime_error("route path must start with '/' at " + routes_file + ":" +
                                     std::to_string(line_number));
        }
        const auto route_key = std::make_pair(static_cast<int>(parsed_method), path);
        if (!seen_routes.insert(route_key).second) {
            throw std::runtime_error("duplicate route at " + routes_file + ":" +
                                     std::to_string(line_number));
        }
        declarations.push_back({parsed_method, std::move(path), std::move(handler), line_number});
    }
    if (input.bad()) {
        throw std::runtime_error("failed while reading routes file: " + routes_file);
    }

    std::lock_guard<std::mutex> lock(routes_mutex_);
    for (const auto& declaration : declarations) {
        if (routes_.find({declaration.method, declaration.path}) != routes_.end()) {
            throw std::runtime_error("route is already registered at " + routes_file + ":" +
                                     std::to_string(declaration.line));
        }
        if (named_handlers_.find(declaration.handler_name) == named_handlers_.end()) {
            throw std::runtime_error("unregistered route handler '" + declaration.handler_name +
                                     "' at " + routes_file + ":" +
                                     std::to_string(declaration.line));
        }
    }
    for (const auto& declaration : declarations) {
        routes_[{declaration.method, declaration.path}] = named_handlers_.at(declaration.handler_name);
    }
}

void Server::start() {
    if (running_.load()) {
        throw std::runtime_error("HTTP server is already running");
    }
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif
    Socket listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == invalid_socket) {
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("Could not create HTTP server socket: " +
                                 std::to_string(socket_error()));
    }
    int reuse_address = 1;
#ifdef _WIN32
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse_address), sizeof(reuse_address));
#else
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));
#endif
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<unsigned short>(port_));
    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
        listen(listener, 64) != 0) {
        const int error = socket_error();
        close_socket(listener);
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("Could not bind/listen on HTTP port " +
                                 std::to_string(port_) + ": " + std::to_string(error));
    }
    running_ = true;
    std::vector<std::thread> workers;
    while (running_.load()) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(listener, &read_set);
        timeval timeout{};
        timeout.tv_usec = 100000;
#ifdef _WIN32
        const int ready = select(0, &read_set, nullptr, nullptr, &timeout);
#else
        const int ready = select(listener + 1, &read_set, nullptr, nullptr, &timeout);
#endif
        if (ready <= 0 || !FD_ISSET(listener, &read_set)) continue;
        sockaddr_in client_address{};
#ifdef _WIN32
        int address_size = sizeof(client_address);
#else
        socklen_t address_size = sizeof(client_address);
#endif
        Socket client = accept(listener, reinterpret_cast<sockaddr*>(&client_address), &address_size);
        if (client == invalid_socket) continue;
        workers.emplace_back([client, this]() {
            try {
                write_response(client, serve_client(client, *this));
            } catch (const std::exception& error) {
                std::cerr << "[rest_api::Server] request handler failed: "
                          << error.what() << std::endl;
                write_response(client, Response(500, "Internal Server Error"));
            } catch (...) {
                std::cerr << "[rest_api::Server] request handler failed with an unknown exception"
                          << std::endl;
                write_response(client, Response(500, "Internal Server Error"));
            }
            close_socket(client);
        });
    }
    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
    close_socket(listener);
#ifdef _WIN32
    WSACleanup();
#endif
}

void Server::stop() {
    running_ = false;
}

} // namespace rest_api
} // namespace ml
