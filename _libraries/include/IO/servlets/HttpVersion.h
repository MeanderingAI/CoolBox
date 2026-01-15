#pragma once
#include <string>

namespace io {
namespace servlets {

enum class HttpVersion {
    HTTP_1,
    HTTP_2,
    HTTP_3,
    UNKNOWN
};

HttpVersion detect_http_version(const std::string& version_str);

} // namespace servlets
} // namespace io
