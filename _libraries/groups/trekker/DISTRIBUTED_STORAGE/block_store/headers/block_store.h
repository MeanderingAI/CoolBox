#ifndef TREKKER_DFS_BLOCK_STORE_H
#define TREKKER_DFS_BLOCK_STORE_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace trekker {
namespace dfs {

// ── Block identity ────────────────────────────────────────────────────────────
// A BlockId is a 256-bit (32-byte) content hash.  Storage is zero-copy:
// callers can supply pre-allocated byte vectors.

using BlockId = std::array<std::uint8_t, 32>;

// Simple hex encoding/decoding for BlockId.
std::string   block_id_to_hex(const BlockId& id);
BlockId       block_id_from_hex(const std::string& hex);

// Compute a deterministic BlockId from raw data (SHA-256 stub using FNV).
BlockId       compute_block_id(const std::uint8_t* data, std::size_t len);

// ── Block metadata ────────────────────────────────────────────────────────────

struct BlockMeta {
    BlockId     id;
    std::size_t size;        // bytes stored
    std::size_t ref_count;   // number of file references
    bool        pinned;      // never evict
};

// ── BlockStore ────────────────────────────────────────────────────────────────
// Content-addressable storage for fixed-size blocks on a single node.
// Thread-safe.

class BlockStore {
public:
    struct Config {
        std::size_t block_size   = 4 * 1024 * 1024;  // 4 MiB
        std::size_t max_blocks   = 1024;              // hard limit; 0 = unlimited
        std::string storage_dir  = "";                // empty = in-memory only
    };

    explicit BlockStore(Config cfg = {});
    ~BlockStore() = default;

    BlockStore(const BlockStore&) = delete;
    BlockStore& operator=(const BlockStore&) = delete;

    // Write a block.  Returns the content-addressed BlockId.
    // If the block already exists its ref_count is incremented.
    BlockId put(const std::uint8_t* data, std::size_t len);
    BlockId put(const std::vector<std::uint8_t>& data) {
        return put(data.data(), data.size());
    }

    // Read a block.  Returns nullopt if not found.
    std::optional<std::vector<std::uint8_t>> get(const BlockId& id) const;

    // Decrement ref_count; remove block when it reaches 0.
    bool release(const BlockId& id);

    // Pin / unpin prevents eviction (e.g. for replicated blocks).
    bool pin(const BlockId& id);
    bool unpin(const BlockId& id);

    // Metadata queries.
    std::optional<BlockMeta>       meta(const BlockId& id) const;
    std::vector<BlockId>           all_block_ids() const;
    std::size_t                    block_count() const;
    std::size_t                    total_bytes() const;

    // Remove all blocks (for testing).
    void clear();

private:
    Config cfg_;
    mutable std::mutex mtx_;

    struct Entry {
        std::vector<std::uint8_t> data;
        BlockMeta                 meta;
    };
    std::unordered_map<std::string, Entry> store_; // hex(id) → Entry
};

// ── BlockMap ──────────────────────────────────────────────────────────────────
// Maps logical file offsets → (node_id, BlockId) pairs.
// A file is split into contiguous equal-sized blocks; the map records
// where each block lives across the cluster.

struct BlockLocation {
    std::string node_id;    // which storage node holds this block
    BlockId     block_id;
    std::size_t offset;     // byte offset within the file
    std::size_t length;     // actual bytes in this block (may be < block_size for last)
};

class BlockMap {
public:
    explicit BlockMap(std::size_t block_size = 4 * 1024 * 1024);

    // Append a new block location for the file.
    void append(BlockLocation loc);

    // Look up which block covers byte offset `off`.
    std::optional<BlockLocation> locate(std::size_t byte_offset) const;

    // All locations in order.
    const std::vector<BlockLocation>& locations() const { return locs_; }

    std::size_t file_size()  const;
    std::size_t block_size() const { return block_size_; }
    std::size_t num_blocks() const { return locs_.size(); }

    void clear();

private:
    std::size_t              block_size_;
    std::vector<BlockLocation> locs_;
};

} // namespace dfs
} // namespace trekker

#endif // TREKKER_DFS_BLOCK_STORE_H
