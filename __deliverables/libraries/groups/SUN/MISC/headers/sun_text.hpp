#pragma once

#include <string>
#include <vector>

namespace sun::misc {

std::string trim(const std::string& input);
std::string to_upper(std::string input);
std::vector<std::string> split_ws(const std::string& input);

}  // namespace sun::misc
