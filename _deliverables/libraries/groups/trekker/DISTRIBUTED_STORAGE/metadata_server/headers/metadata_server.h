#ifndef TREKKER_DFS_METADATA_SERVER_H
#define TREKKER_DFS_METADATA_SERVER_H

#include "block_store.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace trekker {
namespace dfs {

// ── File / directory types ────────────────────────────────────────────────────

enum class NodeType { File, Directory };

struct Stat {
    std::string path;
    NodeType    type;
    std::size_t size;            // bytes (0 for directories)
    std::size_t num_blocks;
    std::uint16_t replication;  // desired replica count
    std::int64_t  mtime;        // Unix epoch seconds
    std::int64_t  atime;
    std::string   owner;
    std::uint16_t mode;         // Unix permission bits
};

// ── Directory tree ────────────────────────────────────────────────────────────
// In-memory inode tree maintained by the MetadataServer.

struct DirEntry {
    std::string   name;
    NodeType      type;
    std::int64_t  inode;
};

class DirTree {
public:
    DirTree();

    // Returns false if path already exists or parent missing.
    bool mkdir(const std::string& path, const std::string& owner = "root",
               std::uint16_t mode = 0755);

    // Create a file entry; block map is stored separately.
    bool mkfile(const std::string& path, const std::string& owner = "root",
                std::uint16_t mode = 0644, std::uint16_t replication = 3);

    // Rename / move (within namespace; does not move data).
    bool rename(const std::string& from, const std::string& to);

    // Remove file or empty directory.
    bool remove(const std::string& path);

    // Stat a path.
    std::optional<Stat> stat(const std::string& path) const;

    // List a directory.  Returns nullopt if not a directory.
    std::optional<std::vector<DirEntry>> listdir(const std::string& path) const;

    // Check existence.
    bool exists(const std::string& path) const;

    // Update file size (called after writes).
    bool set_size(const std::string& path, std::size_t size, std::size_t num_blocks);
    bool touch_mtime(const std::string& path);

private:
    struct INode {
        std::int64_t   inode;
        NodeType       type;
        std::string    owner;
        std::uint16_t  mode;
        std::uint16_t  replication;
        std::size_t    size       = 0;
        std::size_t    num_blocks = 0;
        std::int64_t   mtime      = 0;
        std::int64_t   atime      = 0;
        std::map<std::string, std::int64_t> children; // only for directories
    };

    // Normalise a POSIX path (remove double slashes, trailing slash, etc.)
    std::string normalise(const std::string& p) const;
    // Return the parent path and the last component.
    std::pair<std::string, std::string> split_path(const std::string& p) const;

    std::map<std::string, INode> nodes_; // path → inode
    std::int64_t next_inode_ = 1;
};

// ── MetadataServer ────────────────────────────────────────────────────────────
// Name-node equivalent: owns the DirTree and maps each file path to its
// BlockMap.  Thread-safe via internal mutex.

class MetadataServer {
public:
    MetadataServer() = default;

    // ── Namespace ops ─────────────────────────────────────────────────────
    bool mkdir(const std::string& path, const std::string& owner = "root",
               std::uint16_t mode = 0755);
    bool create(const std::string& path, const std::string& owner = "root",
                std::uint16_t replication = 3);
    bool rename(const std::string& from, const std::string& to);
    bool remove(const std::string& path);

    std::optional<Stat>                   stat(const std::string& path) const;
    std::optional<std::vector<DirEntry>>  listdir(const std::string& path) const;
    bool                                  exists(const std::string& path) const;

    // ── Block map management ──────────────────────────────────────────────
    // Register a new block appended to a file.
    bool append_block(const std::string& path, BlockLocation loc);

    // Get all block locations for a file (for the client to fetch from nodes).
    std::optional<std::vector<BlockLocation>> block_locations(const std::string& path) const;

    // Which storage nodes have a particular block?
    std::vector<std::string> replicas_for(const BlockId& id) const;

    // Register / deregister a storage node heartbeat.
    void register_node(const std::string& node_id, std::size_t capacity_bytes);
    void deregister_node(const std::string& node_id);
    std::vector<std::string> live_nodes() const;

    // Choose the best storage node for a new block (simple round-robin).
    std::string pick_node_for_write() const;

private:
    mutable std::mutex mtx_;
    DirTree            dir_;
    std::map<std::string, BlockMap>   block_maps_;  // path → BlockMap
    std::map<std::string, std::size_t> nodes_;      // node_id → capacity
    mutable std::size_t rr_index_ = 0;
};

} // namespace dfs
} // namespace trekker

#endif // TREKKER_DFS_METADATA_SERVER_H
