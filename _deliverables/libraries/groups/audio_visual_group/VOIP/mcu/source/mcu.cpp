#include "mcu.h"

#include <map>

namespace trekker {
namespace voip {

bool McuBridge::join_room(const std::string& room, const std::string& participant) {
    if (room.empty() || participant.empty()) {
        return false;
    }
    rooms_[room].insert(participant);
    return true;
}

bool McuBridge::leave_room(const std::string& room, const std::string& participant) {
    auto it = rooms_.find(room);
    if (it == rooms_.end()) {
        return false;
    }
    const auto removed = it->second.erase(participant) > 0;
    if (it->second.empty()) {
        rooms_.erase(it);
    }
    return removed;
}

std::size_t McuBridge::participant_count(const std::string& room) const {
    auto it = rooms_.find(room);
    if (it == rooms_.end()) {
        return 0;
    }
    return it->second.size();
}

} // namespace voip
} // namespace trekker
