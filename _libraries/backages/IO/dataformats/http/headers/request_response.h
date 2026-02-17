
#pragma once

#include <string>
#include <map>
#include <vector>
#include <variant>

enum class HeaderKey {
    ContentType,
    ContentLength,
    Connection,
    CacheControl,
    Location,
    SetCookie,
    Server,
    Date,
    Accept,
    AcceptEncoding,
    AcceptLanguage,
    UserAgent,
    Host,
    Authorization,
    Cookie,
    Custom
};

inline std::string header_key_to_string(HeaderKey key) {
    switch (key) {
        case HeaderKey::ContentType: return "Content-Type";
        case HeaderKey::ContentLength: return "Content-Length";
        case HeaderKey::Connection: return "Connection";
        case HeaderKey::CacheControl: return "Cache-Control";
        case HeaderKey::Location: return "Location";
        case HeaderKey::SetCookie: return "Set-Cookie";
        case HeaderKey::Server: return "Server";
        case HeaderKey::Date: return "Date";
        case HeaderKey::Accept: return "Accept";
        case HeaderKey::AcceptEncoding: return "Accept-Encoding";
        case HeaderKey::AcceptLanguage: return "Accept-Language";
        case HeaderKey::UserAgent: return "User-Agent";
        case HeaderKey::Host: return "Host";
        case HeaderKey::Authorization: return "Authorization";
        case HeaderKey::Cookie: return "Cookie";
        case HeaderKey::Custom: return "Custom";
        default: return "Unknown";
    }
}

using HeaderKeyType = std::variant<HeaderKey, std::string>;

// Convert header string to HeaderKey enum if possible
inline HeaderKey header_key_from_string(const std::string& key) {
    if (key == "Content-Type") return HeaderKey::ContentType;
    if (key == "Content-Length") return HeaderKey::ContentLength;
    if (key == "Connection") return HeaderKey::Connection;
    if (key == "Cache-Control") return HeaderKey::CacheControl;
    if (key == "Location") return HeaderKey::Location;
    if (key == "Set-Cookie") return HeaderKey::SetCookie;
    if (key == "Server") return HeaderKey::Server;
    if (key == "Date") return HeaderKey::Date;
    if (key == "Accept") return HeaderKey::Accept;
    if (key == "Accept-Encoding") return HeaderKey::AcceptEncoding;
    if (key == "Accept-Language") return HeaderKey::AcceptLanguage;
    if (key == "User-Agent") return HeaderKey::UserAgent;
    if (key == "Host") return HeaderKey::Host;
    if (key == "Authorization") return HeaderKey::Authorization;
    if (key == "Cookie") return HeaderKey::Cookie;
    return HeaderKey::Custom;
}


struct Request {
    std::string method;
    std::string uri;
    std::map<HeaderKeyType, std::string> headers;
    std::string body;

    // Parse a raw HTTP request string into a Request object
    static Request from_string(const std::string& buffer) {
        Request req;
        size_t method_end = buffer.find(' ');
        if (method_end != std::string::npos) {
            req.method = buffer.substr(0, method_end);
            size_t path_start = method_end + 1;
            size_t path_end = buffer.find(' ', path_start);
            if (path_end != std::string::npos) {
                req.uri = buffer.substr(path_start, path_end - path_start);
            }
        }
        // Parse headers and body
        size_t header_start = buffer.find("\r\n") + 2;
        size_t body_pos = buffer.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            // Parse headers
            size_t pos = header_start;
            while (pos < body_pos) {
                size_t line_end = buffer.find("\r\n", pos);
                if (line_end == std::string::npos || line_end > body_pos) break;
                std::string line = buffer.substr(pos, line_end - pos);
                size_t colon = line.find(":");
                if (colon != std::string::npos) {
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    HeaderKey hkey = header_key_from_string(key);
                    if (hkey == HeaderKey::Custom)
                        req.headers[key] = value;
                    else
                        req.headers[hkey] = value;
                }
                pos = line_end + 2;
            }
            req.body = buffer.substr(body_pos + 4);
        }
        return req;
    }
};

struct Response {
    int status_code;
    std::map<HeaderKeyType, std::string> headers;
    std::string body;

    static Response ok(const std::string& body) {
        Response r;
        r.status_code = 200;
        r.body = body;
        r.headers[HeaderKey::ContentLength] = std::to_string(body.size());
        return r;
    }
    static Response not_found() {
        Response r;
        r.status_code = 404;
        r.body = "Not Found";
        r.headers[HeaderKey::ContentLength] = std::to_string(r.body.size());
        return r;
    }

    std::string to_string() const {
        std::string status_line = "HTTP/1.1 " + std::to_string(status_code) + " " + reason_phrase() + "\r\n";
        std::string header_lines;
        for (const auto& [key, value] : headers) {
            std::string key_str = std::holds_alternative<HeaderKey>(key) ? header_key_to_string(std::get<HeaderKey>(key)) : std::get<std::string>(key);
            header_lines += key_str + ": " + value + "\r\n";
        }
        return status_line + header_lines + "\r\n" + body;
    }

    std::string reason_phrase() const {
        switch (status_code) {
            case 200: return "OK";
            case 201: return "Created";
            case 204: return "No Content";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 500: return "Internal Server Error";
            default: return "Unknown";
        }
    }
};

