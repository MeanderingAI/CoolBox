#include "sun_table.hpp"

namespace sun::data {

void SunTable::upsert(std::string key, std::string value) {
    kv_[std::move(key)] = std::move(value);
}

std::optional<std::string> SunTable::get(const std::string& key) const {
    const auto it = kv_.find(key);
    if (it == kv_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool SunTable::erase(const std::string& key) {
    return kv_.erase(key) > 0;
}

std::size_t SunTable::size() const {
    return kv_.size();
}

const std::unordered_map<std::string, std::string>& SunTable::items() const {
    return kv_;
}

}  // namespace sun::data
