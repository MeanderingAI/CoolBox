#ifndef IO_HTTP_SERVER_REQUEST_BUILDER_H
#define IO_HTTP_SERVER_REQUEST_BUILDER_H
#include "IO/dataformats/http/request_response.h"
#include <string>

namespace io {
namespace http_server {

class RequestBuilder {
public:
    // Very basic HTTP request parser from raw buffer
    static Request from_buffer(const std::string& buffer) {
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
                    req.headers[key] = value;
                }
                pos = line_end + 2;
            }
            req.body = buffer.substr(body_pos + 4);
        }
        return req;
    }
};

} // namespace http_server
} // namespace io

#endif // IO_HTTP_SERVER_REQUEST_BUILDER_H

