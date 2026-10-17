#ifndef ML_DEEP_LEARNING_SCALING_COLLECTIVE_H
#define ML_DEEP_LEARNING_SCALING_COLLECTIVE_H

#include <cstddef>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// Half-open slice [offset, offset + count) of a flattened parameter vector.
struct ShardRange {
    size_t offset = 0;
    size_t count = 0;

    size_t end() const { return offset + count; }
    bool contains(size_t index) const { return index >= offset && index < end(); }
};

/// Splits @p total elements across @p world_size ranks, giving the leading
/// ranks one extra element when the division is uneven.
std::vector<ShardRange> partition_evenly(size_t total, size_t world_size);

/// In-process stand-in for an NCCL/MPI process group.
///
/// Every collective takes one buffer per simulated rank so that ZeRO, Domino
/// and 1-bit LAMB can be exercised deterministically in a single process while
/// still accounting for the communication volume each algorithm generates.
class CollectiveGroup {
public:
    explicit CollectiveGroup(size_t world_size);

    size_t world_size() const { return world_size_; }

    /// Sums @p buffers element-wise and writes the result back to every rank.
    void all_reduce(std::vector<std::vector<double>>& buffers);

    /// Sums @p buffers element-wise, then hands rank r only its own shard.
    void reduce_scatter(const std::vector<std::vector<double>>& buffers,
                        std::vector<std::vector<double>>& shards);

    /// Concatenates the per-rank @p shards into a full buffer on every rank.
    void all_gather(const std::vector<std::vector<double>>& shards,
                    std::vector<std::vector<double>>& gathered);

    /// Copies the buffer owned by @p root to every rank.
    void broadcast(const std::vector<double>& source,
                   size_t root,
                   std::vector<std::vector<double>>& buffers);

    /// Bytes transferred by collectives issued so far, using the standard
    /// ring-algorithm cost model (2(N-1)/N per all-reduce element).
    double bytes_moved() const { return bytes_moved_; }
    void reset_counters() { bytes_moved_ = 0.0; }

private:
    void account(size_t elements, double factor);

    size_t world_size_;
    double bytes_moved_ = 0.0;
};

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_COLLECTIVE_H
