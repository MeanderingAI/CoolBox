#include "replication.h"
#include <algorithm>
#include <random>
#include <stdexcept>

namespace trekker {
namespace dfs {

// ── ReplicationPlanner ────────────────────────────────────────────────────────

ReplicationPlanner::ReplicationPlanner(ReplicationConfig cfg)
    : cfg_(std::move(cfg)) {}

std::vector<std::string> ReplicationPlanner::plan(
    const std::vector<std::string>& live_nodes,
    const std::vector<std::string>& existing,
    int needed) const {
    // Filter out nodes that already have the block.
    std::vector<std::string> candidates;
    for (const auto& n : live_nodes) {
        if (std::find(existing.begin(), existing.end(), n) == existing.end())
            candidates.push_back(n);
    }

    std::vector<std::string> result;
    if (candidates.empty() || needed <= 0) return result;

    switch (cfg_.strategy) {
        case PlacementStrategy::RoundRobin: {
            for (int i = 0; i < needed && !candidates.empty(); ++i) {
                const std::size_t idx = rr_cursor_ % candidates.size();
                result.push_back(candidates[idx]);
                ++rr_cursor_;
            }
            break;
        }
        case PlacementStrategy::RackAware: {
            // Group candidates by rack (prefix before first '-').
            std::vector<std::string> used_racks;
            for (const auto& n : candidates) {
                if (static_cast<int>(result.size()) >= needed) break;
                const std::string rack = n.substr(0, n.find('-'));
                if (std::find(used_racks.begin(), used_racks.end(), rack) == used_racks.end()) {
                    result.push_back(n);
                    used_racks.push_back(rack);
                }
            }
            // Backfill from any remaining candidate if still needed.
            for (const auto& n : candidates) {
                if (static_cast<int>(result.size()) >= needed) break;
                if (std::find(result.begin(), result.end(), n) == result.end())
                    result.push_back(n);
            }
            break;
        }
        case PlacementStrategy::Random: {
            std::shuffle(candidates.begin(), candidates.end(),
                         std::mt19937{std::random_device{}()});
            for (int i = 0; i < needed && i < static_cast<int>(candidates.size()); ++i)
                result.push_back(candidates[i]);
            break;
        }
    }
    return result;
}

// ── BlockReplicator ───────────────────────────────────────────────────────────

BlockReplicator::BlockReplicator(BlockStoreProvider provider, ReplicationConfig cfg)
    : provider_(std::move(provider)), cfg_(std::move(cfg)), planner_(cfg_) {}

int BlockReplicator::replicate(const BlockId& id,
                               const std::string& source_node,
                               const std::vector<std::string>& target_nodes) {
    BlockStore* src = provider_(source_node);
    if (!src) return 0;

    auto data_opt = src->get(id);
    if (!data_opt) return 0;

    int copied = 0;
    for (const auto& target : target_nodes) {
        BlockStore* dst = provider_(target);
        if (!dst) continue;
        dst->put(data_opt->data(), data_opt->size());
        ++copied;
    }
    return copied;
}

int BlockReplicator::rebalance(const BlockId& id,
                               const std::string& source_node,
                               const std::vector<std::string>& live_nodes,
                               const std::vector<std::string>& existing_nodes) {
    const int have   = static_cast<int>(existing_nodes.size());
    const int needed = cfg_.replication_factor - have;
    if (needed <= 0) return 0;

    const std::vector<std::string> targets =
        planner_.plan(live_nodes, existing_nodes, needed);
    return replicate(id, source_node, targets);
}

std::vector<std::string> BlockReplicator::verify(
    const BlockId& id,
    const std::vector<std::string>& node_ids) {
    std::vector<std::string> missing;
    for (const auto& nid : node_ids) {
        BlockStore* store = provider_(nid);
        if (!store || !store->get(id))
            missing.push_back(nid);
    }
    return missing;
}

} // namespace dfs
} // namespace trekker
