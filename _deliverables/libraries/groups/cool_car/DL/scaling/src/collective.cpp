#include "collective.h"

#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

constexpr double kBytesPerElement = 4.0; // fp16 gradients are accumulated in fp32.

void require_uniform(const std::vector<std::vector<double>>& buffers, size_t world_size) {
    if (buffers.size() != world_size) {
        throw std::invalid_argument("collective: one buffer per rank is required");
    }
    if (buffers.empty()) {
        return;
    }
    const size_t elements = buffers.front().size();
    for (const auto& buffer : buffers) {
        if (buffer.size() != elements) {
            throw std::invalid_argument("collective: all rank buffers must have equal size");
        }
    }
}

} // namespace

std::vector<ShardRange> partition_evenly(size_t total, size_t world_size) {
    if (world_size == 0) {
        throw std::invalid_argument("partition_evenly: world_size must be > 0");
    }
    std::vector<ShardRange> ranges(world_size);
    const size_t base = total / world_size;
    const size_t remainder = total % world_size;
    size_t offset = 0;
    for (size_t rank = 0; rank < world_size; ++rank) {
        const size_t count = base + (rank < remainder ? 1 : 0);
        ranges[rank] = ShardRange{offset, count};
        offset += count;
    }
    return ranges;
}

CollectiveGroup::CollectiveGroup(size_t world_size) : world_size_(world_size) {
    if (world_size == 0) {
        throw std::invalid_argument("CollectiveGroup: world_size must be > 0");
    }
}

void CollectiveGroup::account(size_t elements, double factor) {
    bytes_moved_ += static_cast<double>(elements) * kBytesPerElement * factor;
}

void CollectiveGroup::all_reduce(std::vector<std::vector<double>>& buffers) {
    require_uniform(buffers, world_size_);
    if (buffers.empty() || buffers.front().empty()) {
        return;
    }

    const size_t elements = buffers.front().size();
    std::vector<double> sum(elements, 0.0);
    for (const auto& buffer : buffers) {
        for (size_t i = 0; i < elements; ++i) {
            sum[i] += buffer[i];
        }
    }
    for (auto& buffer : buffers) {
        buffer = sum;
    }

    const double n = static_cast<double>(world_size_);
    account(elements, 2.0 * (n - 1.0) / n);
}

void CollectiveGroup::reduce_scatter(const std::vector<std::vector<double>>& buffers,
                                     std::vector<std::vector<double>>& shards) {
    require_uniform(buffers, world_size_);
    const size_t elements = buffers.empty() ? 0 : buffers.front().size();

    std::vector<double> sum(elements, 0.0);
    for (const auto& buffer : buffers) {
        for (size_t i = 0; i < elements; ++i) {
            sum[i] += buffer[i];
        }
    }

    const auto ranges = partition_evenly(elements, world_size_);
    shards.assign(world_size_, {});
    for (size_t rank = 0; rank < world_size_; ++rank) {
        const ShardRange& range = ranges[rank];
        shards[rank].assign(sum.begin() + static_cast<std::ptrdiff_t>(range.offset),
                            sum.begin() + static_cast<std::ptrdiff_t>(range.end()));
    }

    const double n = static_cast<double>(world_size_);
    account(elements, (n - 1.0) / n);
}

void CollectiveGroup::all_gather(const std::vector<std::vector<double>>& shards,
                                 std::vector<std::vector<double>>& gathered) {
    if (shards.size() != world_size_) {
        throw std::invalid_argument("all_gather: one shard per rank is required");
    }

    std::vector<double> full;
    for (const auto& shard : shards) {
        full.insert(full.end(), shard.begin(), shard.end());
    }
    gathered.assign(world_size_, full);

    const double n = static_cast<double>(world_size_);
    account(full.size(), (n - 1.0) / n);
}

void CollectiveGroup::broadcast(const std::vector<double>& source,
                                size_t root,
                                std::vector<std::vector<double>>& buffers) {
    if (root >= world_size_) {
        throw std::invalid_argument("broadcast: root rank is out of range");
    }
    buffers.assign(world_size_, source);
    account(source.size(), static_cast<double>(world_size_ - 1));
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
