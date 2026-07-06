#ifndef TREKKER_DFS_REPLICATION_H
#define TREKKER_DFS_REPLICATION_H

#include "block_store.h"
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace trekker {
namespace dfs {

// ── Replication policy ────────────────────────────────────────────────────────

enum class PlacementStrategy {
    RoundRobin,     // cycle through nodes in order
    RackAware,      // prefer nodes on different racks (rack = first segment of node_id)
    Random,         // uniform random selection
};

struct ReplicationConfig {
    int               replication_factor = 3;      // target number of replicas
    PlacementStrategy strategy           = PlacementStrategy::RoundRobin;
    bool              verify_on_read     = false;  // re-read and compare on every fetch
};

// ── ReplicationPlanner ────────────────────────────────────────────────────────
// Selects which storage nodes should receive a new block replica.

class ReplicationPlanner {
public:
    explicit ReplicationPlanner(ReplicationConfig cfg = {});

    // Given a list of all live nodes and nodes that already have the block,
    // return a list of nodes that should receive new replicas.
    std::vector<std::string> plan(const std::vector<std::string>& live_nodes,
                                  const std::vector<std::string>& existing,
                                  int needed) const;

private:
    ReplicationConfig cfg_;
    mutable std::size_t rr_cursor_ = 0;
};

// ── BlockReplicator ───────────────────────────────────────────────────────────
// Coordinates copying a block from a source node to target nodes.
// In a real system this would issue RPCs; here it operates on in-process
// BlockStore instances (extensible via a BlockStoreProvider callback).

using BlockStoreProvider = std::function<BlockStore*(const std::string& node_id)>;

class BlockReplicator {
public:
    explicit BlockReplicator(BlockStoreProvider provider,
                             ReplicationConfig  cfg = {});

    // Replicate `id` from `source_node` to `target_nodes`.
    // Returns the number of successful copies made.
    int replicate(const BlockId& id,
                  const std::string& source_node,
                  const std::vector<std::string>& target_nodes);

    // Ensure block `id` meets the replication factor across `live_nodes`.
    // Reads current replica locations from `existing_nodes`.
    int rebalance(const BlockId& id,
                  const std::string& source_node,
                  const std::vector<std::string>& live_nodes,
                  const std::vector<std::string>& existing_nodes);

    // Verify block integrity on all `node_ids`; returns node_ids where the
    // block is missing or corrupted.
    std::vector<std::string> verify(const BlockId& id,
                                    const std::vector<std::string>& node_ids);

    const ReplicationConfig& config() const { return cfg_; }

private:
    BlockStoreProvider provider_;
    ReplicationConfig  cfg_;
    ReplicationPlanner planner_;
};

} // namespace dfs
} // namespace trekker

#endif // TREKKER_DFS_REPLICATION_H
