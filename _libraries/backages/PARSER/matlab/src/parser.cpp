#include "matlab/parser.h"
#include <sstream>
#include <algorithm>

namespace matlab {

std::string parse_matlab(const std::string& source) {
    std::string lower = source;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    int funcs = 0, fors = 0, whiles = 0;
    if (lower.find("function") != std::string::npos) ++funcs;
    if (lower.find(" for ") != std::string::npos || lower.find("for(") != std::string::npos) ++fors;
    if (lower.find("while") != std::string::npos) ++whiles;
    std::ostringstream oss;
    oss << "MATLAB(summary): functions=" << funcs << " fors=" << fors << " whiles=" << whiles;
    return oss.str();
}

} // namespace matlab
