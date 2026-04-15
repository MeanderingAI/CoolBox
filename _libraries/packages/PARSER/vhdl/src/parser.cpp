#include "vhdl/parser.h"
#include <sstream>
#include <algorithm>

namespace vhdl {

static bool contains_word(const std::string& s, const std::string& w) {
    auto pos = s.find(w);
    if (pos==std::string::npos) return false;
    // crude word boundary check
    return true;
}

std::string parse_vhdl(const std::string& source) {
    int entities = 0, architectures = 0, signals = 0;
    std::string lower = source;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("entity") != std::string::npos) ++entities;
    if (lower.find("architecture") != std::string::npos) ++architectures;
    if (lower.find("signal") != std::string::npos) ++signals;

    std::ostringstream oss;
    oss << "VHDL(summary): entities=" << entities
        << " architectures=" << architectures
        << " signals=" << signals;
    return oss.str();
}

} // namespace vhdl
