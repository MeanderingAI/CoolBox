#pragma once

#include <string>

namespace vhdl {

// Very small VHDL parser helper: returns a short summary/AST-like string
// for simple use in unit tests and downstream tools. This is intentionally
// lightweight and not a full VHDL implementation.
std::string parse_vhdl(const std::string& source);

} // namespace vhdl
