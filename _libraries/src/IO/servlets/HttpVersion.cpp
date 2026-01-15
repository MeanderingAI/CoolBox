#include "IO/servlets/HttpVersion.h"

namespace io {
namespace servlets {

HttpVersion detect_http_version(const std::string& version_str) {
    if (version_str.find("2") != std::string::npos) return HttpVersion::HTTP_2;
    if (version_str.find("3") != std::string::npos) return HttpVersion::HTTP_3;
    if (version_str.find("1") != std::string::npos) return HttpVersion::HTTP_1;
    return HttpVersion::UNKNOWN;
}

} // namespace servlets
} // namespace io
