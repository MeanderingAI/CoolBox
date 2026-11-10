#include "zero_infinity.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace scaling {

std::string to_string(MemoryTier tier) {
    switch (tier) {
        case MemoryTier::kGpu: return "gpu";
        case MemoryTier::kCpu: return "cpu";
        case MemoryTier::kNvme: return "nvme";
    }
    return "unknown";
}

InfinityOffloadEngine::InfinityOffloadEngine(std::vector<DeviceTier> tiers)
    : tiers_(std::move(tiers)) {
    if (tiers_.empty()) {
        throw std::invalid_argument("InfinityOffloadEngine: at least one tier is required");
    }
    // Fastest first, which is also the preferred placement order.
    std::sort(tiers_.begin(), tiers_.end(), [](const DeviceTier& a, const DeviceTier& b) {
        return a.bandwidth_bytes_per_second > b.bandwidth_bytes_per_second;
    });
}

const DeviceTier* InfinityOffloadEngine::find(MemoryTier tier) const {
    for (const auto& candidate : tiers_) {
        if (candidate.tier == tier) {
            return &candidate;
        }
    }
    return nullptr;
}

OffloadPlan InfinityOffloadEngine::plan(const ZeroMemoryEstimate& estimate) const {
    // Ordered by access frequency: parameters are touched every forward and
    // backward pass, optimizer state only once per step, so it is demoted first.
    const std::vector<StatePlacement> requests = {
        {"parameters", MemoryTier::kGpu, estimate.parameter_bytes},
        {"gradients", MemoryTier::kGpu, estimate.gradient_bytes},
        {"optimizer", MemoryTier::kGpu, estimate.optimizer_bytes}
    };

    std::vector<double> remaining;
    remaining.reserve(tiers_.size());
    for (const auto& tier : tiers_) {
        remaining.push_back(tier.capacity_bytes);
    }

    OffloadPlan result;
    result.fits = true;
    for (auto request : requests) {
        bool placed = false;
        for (size_t i = 0; i < tiers_.size(); ++i) {
            if (remaining[i] >= request.bytes) {
                remaining[i] -= request.bytes;
                request.tier = tiers_[i].tier;
                if (tiers_[i].bandwidth_bytes_per_second > 0.0) {
                    result.estimated_transfer_seconds +=
                        request.bytes / tiers_[i].bandwidth_bytes_per_second;
                }
                if (tiers_[i].tier == MemoryTier::kGpu) {
                    result.resident_gpu_bytes += request.bytes;
                }
                placed = true;
                break;
            }
        }
        if (!placed) {
            result.fits = false;
            request.tier = tiers_.back().tier;
        }
        result.placements.push_back(request);
    }
    return result;
}

double InfinityOffloadEngine::aggregate_bandwidth(MemoryTier tier, size_t data_parallel_degree) const {
    const DeviceTier* device = find(tier);
    if (device == nullptr || data_parallel_degree == 0) {
        return 0.0;
    }
    return device->bandwidth_bytes_per_second * static_cast<double>(data_parallel_degree);
}

std::vector<size_t> InfinityOffloadEngine::bandwidth_centric_partition(
    size_t bytes, size_t data_parallel_degree) const {
    const auto ranges = partition_evenly(bytes, data_parallel_degree);
    std::vector<size_t> slices;
    slices.reserve(ranges.size());
    for (const auto& range : ranges) {
        slices.push_back(range.count);
    }
    return slices;
}

std::vector<ShardRange> InfinityOffloadEngine::memory_centric_tiles(size_t rows,
                                                                    size_t bytes_per_row,
                                                                    double tile_budget_bytes) {
    if (bytes_per_row == 0) {
        throw std::invalid_argument("memory_centric_tiles: bytes_per_row must be > 0");
    }
    if (tile_budget_bytes < static_cast<double>(bytes_per_row)) {
        throw std::invalid_argument("memory_centric_tiles: budget cannot hold a single row");
    }

    const size_t rows_per_tile = static_cast<size_t>(tile_budget_bytes / static_cast<double>(bytes_per_row));
    std::vector<ShardRange> tiles;
    for (size_t offset = 0; offset < rows; offset += rows_per_tile) {
        tiles.push_back(ShardRange{offset, std::min(rows_per_tile, rows - offset)});
    }
    return tiles;
}

double InfinityOffloadEngine::overlapped_seconds(double compute_seconds,
                                                 double transfer_seconds,
                                                 bool overlap) {
    return overlap ? std::max(compute_seconds, transfer_seconds)
                   : compute_seconds + transfer_seconds;
}

double InfinityOffloadEngine::efficiency(double arithmetic_intensity,
                                         double bandwidth_bytes_per_second,
                                         double peak_flops) {
    const double supplied = arithmetic_intensity * bandwidth_bytes_per_second;
    if (supplied <= 0.0 || peak_flops <= 0.0) {
        return 0.0;
    }
    return supplied / (supplied + peak_flops);
}

double InfinityOffloadEngine::required_bandwidth(double arithmetic_intensity,
                                                 double peak_flops,
                                                 double target_efficiency) {
    if (arithmetic_intensity <= 0.0) {
        throw std::invalid_argument("required_bandwidth: arithmetic_intensity must be > 0");
    }
    if (target_efficiency <= 0.0 || target_efficiency >= 1.0) {
        throw std::invalid_argument("required_bandwidth: target_efficiency must be in (0, 1)");
    }
    return (peak_flops * target_efficiency) / (arithmetic_intensity * (1.0 - target_efficiency));
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
