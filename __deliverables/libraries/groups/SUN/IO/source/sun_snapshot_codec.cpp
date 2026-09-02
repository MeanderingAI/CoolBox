#include "sun_snapshot_codec.hpp"

#include <sstream>

#include "sun_text.hpp"

namespace sun::io {

std::string encode_snapshot(const sun::data::SunTable& table) {
    std::ostringstream out;
    for (const auto& [key, value] : table.items()) {
        out << key << '=' << value << '\n';
    }
    return out.str();
}

sun::data::SunTable decode_snapshot(const std::string& snapshot_text) {
    sun::data::SunTable table;
    std::istringstream in(snapshot_text);
    std::string line;
    while (std::getline(in, line)) {
        const std::string trimmed = sun::misc::trim(line);
        if (trimmed.empty()) {
            continue;
        }

        const std::size_t sep = trimmed.find('=');
        if (sep == std::string::npos) {
            continue;
        }

        table.upsert(trimmed.substr(0, sep), trimmed.substr(sep + 1));
    }
    return table;
}

}  // namespace sun::io
