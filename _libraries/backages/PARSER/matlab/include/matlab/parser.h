#pragma once

#include <string>

namespace matlab {

// Lightweight MATLAB parser helper: returns a short summary string
// describing simple constructs found in the source.
std::string parse_matlab(const std::string& source);

} // namespace matlab
