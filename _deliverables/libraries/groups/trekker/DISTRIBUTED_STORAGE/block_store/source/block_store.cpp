#include "block_store.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace trekker {
namespace dfs {

// ── BlockId helpers ───────────────────────────────────────────────────────────

std::string block_id_to_hex(const BlockId& id) {
    std::ostringstream oss;
    for (auto b : id) oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

BlockId block_id_from_hex(const std::string& hex) {
    BlockId id{};
    if (hex.size() != 64) return id;
    for (std::size_t i = 0; i < 32; ++i) {
        id[i] = static_cast<std::uint8_t>(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
    }
    return id;
}

// Simple FNV-1a-based 256-bit-like hash (not a real SHA-256).
BlockId compute_block_id(const std::uint8_t* data, std::size_t len) {
    BlockId id{};
    // Seed four 64-bit FNV values for 256 bits of output.
    const std::uint64_t FNV_PRIME  = 0x00000100000001B3ULL;
    std::uint64_t h[4] = {
        0xcbf29ce484222325ULL,
        0xabc29ce484222777ULL,
        0x9ef29ce484222999ULL,
        0xdef29ce484222BBBULL,
    };
    for (std::size_t i = 0; i < len; ++i) {
        h[i % 4] ^= static_cast<std::uint64_t>(data[i]);
        h[i % 4] *= FNV_PRIME;
        // Cross-mix
        h[(i + 1) % 4] ^= h[i % 4] >> 17;
    }
    for (int k = 0; k < 4; ++k) {
        std::memcpy(id.data() + k * 8, &h[k], 8);
    }
    return id;
}

// ── BlockStore ────────────────────────────────────────────────────────────────

BlockStore::BlockStore(Config cfg) : cfg_(std::move(cfg)) {}

BlockId BlockStore::put(const std::uint8_t* data, std::size_t len) {
    const BlockId id = compute_block_id(data, len);
    const std::string hex = block_id_to_hex(id);
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = store_.find(hex);
    if (it != store_.end()) {
        ++it->second.meta.ref_count;
        return id;
    }
    if (cfg_.max_blocks > 0 && store_.size() >= cfg_.max_blocks)
        throw std::runtime_error("BlockStore: capacity exceeded");
    Entry entry;
    entry.data.assign(data, data + len);
    entry.meta = {id, len, 1, false};
    store_.emplace(hex, std::move(entry));
    return id;
}

std::optional<std::vector<std::uint8_t>> BlockStore::get(const BlockId& id) const {
    const std::string hex = block_id_to_hex(id);
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = store_.find(hex);
    if (it == store_.end()) return std::nullopt;
    it->second.meta.ref_count; // touch atime conceptually
    return it->second.data;
}

bool BlockStore::release(const BlockId& id) {
    const std::string hex = block_id_to_hex(id);
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = store_.find(hex);
    if (it == store_.end()) return false;
    if (it->second.meta.ref_count > 0) --it->second.meta.ref_count;
    if (it->second.meta.ref_count == 0 && !it->second.meta.pinned)
        store_.erase(it);
    return true;
}

bool BlockStore::pin(const BlockId& id) {
    const std::string hex = block_id_to_hex(id);
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = store_.find(hex);
    if (it == store_.end()) return false;
    it->second.meta.pinned = true;
    return true;
}

bool BlockStore::unpin(const BlockId& id) {
    const std::string hex = block_id_to_hex(id);
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = store_.find(hex);
    if (it == store_.end()) return false;
    it->second.meta.pinned = false;
    return true;
}

std::optional<BlockMeta> BlockStore::meta(const BlockId& id) const {
    const std::string hex = block_id_to_hex(id);
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = store_.find(hex);
    if (it == store_.end()) return std::nullopt;
    return it->second.meta;
}

std::vector<BlockId> BlockStore::all_block_ids() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<BlockId> ids;
    ids.reserve(store_.size());
    for (const auto& kv : store_) ids.push_back(kv.second.meta.id);
    return ids;
}

std::size_t BlockStore::block_count() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return store_.size();
}

std::size_t BlockStore::total_bytes() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::size_t n = 0;
    for (const auto& kv : store_) n += kv.second.meta.size;
    return n;
}

void BlockStore::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    store_.clear();
}

} // namespace dfs
} // namespace trekker
