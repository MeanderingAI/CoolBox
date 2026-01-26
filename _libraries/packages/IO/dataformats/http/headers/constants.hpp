
// HTTP constants in global namespace
#pragma once
#include <string>

namespace http_constants {
    constexpr int STATUS_OK = 200;
    constexpr int STATUS_NOT_FOUND = 404;
    constexpr int STATUS_INTERNAL_ERROR = 500;

    constexpr const char* STATUS_TEXT_OK = "OK";
    constexpr const char* STATUS_TEXT_NOT_FOUND = "Not Found";
    constexpr const char* STATUS_TEXT_INTERNAL_ERROR = "Internal Server Error";

    constexpr const char* HEADER_CONTENT_TYPE = "Content-Type";
    constexpr const char* CONTENT_TYPE_JSON = "application/json";
    constexpr const char* CONTENT_TYPE_HTML = "text/html";
    constexpr const char* CONTENT_TYPE_TEXT = "text/plain";
    constexpr const char* CONTENT_TYPE_OCTET = "application/octet-stream";
}