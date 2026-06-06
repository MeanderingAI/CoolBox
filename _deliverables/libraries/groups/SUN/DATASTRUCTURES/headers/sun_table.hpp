#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace sun::data {

class SunTable {
public:
    void upsert(std::string key, std::string value);
    std::optional<std::string> get(const std::string& key) const;
    bool erase(const std::string& key);
    std::size_t size() const;

    const std::unordered_map<std::string, std::string>& items() const;

private:
    std::unordered_map<std::string, std::string> kv_;
};

}  // namespace sun::data
