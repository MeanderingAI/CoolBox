#include "block_store.h"

namespace trekker {
namespace dfs {

// ── BlockMap ──────────────────────────────────────────────────────────────────

BlockMap::BlockMap(std::size_t block_size) : block_size_(block_size) {}

void BlockMap::append(BlockLocation loc) {
    locs_.push_back(std::move(loc));
}

std::optional<BlockLocation> BlockMap::locate(std::size_t byte_offset) const {
    for (const auto& loc : locs_) {
        if (byte_offset >= loc.offset && byte_offset < loc.offset + loc.length)
            return loc;
    }
    return std::nullopt;
}

std::size_t BlockMap::file_size() const {
    if (locs_.empty()) return 0;
    const auto& last = locs_.back();
    return last.offset + last.length;
}

void BlockMap::clear() {
    locs_.clear();
}

} // namespace dfs
} // namespace trekker
