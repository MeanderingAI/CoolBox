#pragma once

#include <map>
#include <set>
#include <string>

namespace trekker {
namespace voip {

class McuBridge {
public:
    bool join_room(const std::string& room, const std::string& participant);
    bool leave_room(const std::string& room, const std::string& participant);
    std::size_t participant_count(const std::string& room) const;

private:
    std::map<std::string, std::set<std::string>> rooms_;
};

} // namespace voip
} // namespace trekker
