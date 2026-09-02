#pragma once

#include <string>

#include "sun_table.hpp"

namespace sun::io {

std::string encode_snapshot(const sun::data::SunTable& table);
sun::data::SunTable decode_snapshot(const std::string& snapshot_text);

}  // namespace sun::io
