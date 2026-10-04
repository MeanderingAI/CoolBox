#ifndef ML_DEEP_LEARNING_SCALING_ZERO_INFINITY_H
#define ML_DEEP_LEARNING_SCALING_ZERO_INFINITY_H

#include "collective.h"
#include "zero.h"

#include <cstddef>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// @brief ZeRO-Infinity heterogeneous offload planning.
///
/// Implements the infinity offload engine, bandwidth-centric partitioning,
/// memory-centric tiling and overlap-centric scheduling of
/// Rajbhandari et al., "ZeRO-Infinity: Breaking the GPU Memory Wall for Extreme
/// Scale Deep Learning", SC '21 (arXiv:2104.07857), citation key
/// @c rajbhandari_zero_infinity_2021.
enum class MemoryTier {
    kGpu = 0,
    kCpu = 1,
    kNvme = 2
};

std::string to_string(MemoryTier tier);

/// One level of the memory hierarchy available to a single accelerator.
struct DeviceTier {
    MemoryTier tier = MemoryTier::kGpu;
    double capacity_bytes = 0.0;
    /// Achievable bandwidth between this tier and the accelerator's compute units.
    double bandwidth_bytes_per_second = 0.0;
};

/// Where one category of training state lives after planning.
struct StatePlacement {
    std::string state;   ///< "parameters", "gradients" or "optimizer".
    MemoryTier tier = MemoryTier::kGpu;
    double bytes = 0.0;
};

struct OffloadPlan {
    std::vector<StatePlacement> placements;
    bool fits = false;
    /// Time to move the whole working set through the hierarchy once.
    double estimated_transfer_seconds = 0.0;
    double resident_gpu_bytes = 0.0;
};

class InfinityOffloadEngine {
public:
    explicit InfinityOffloadEngine(std::vector<DeviceTier> tiers);

    const std::vector<DeviceTier>& tiers() const { return tiers_; }

    /// Places optimizer state first in the slowest tier and parameters last in
    /// the fastest, mirroring the access-frequency ordering of Section 6.
    OffloadPlan plan(const ZeroMemoryEstimate& estimate) const;

    /// Bandwidth-centric partitioning: instead of one owner broadcasting a
    /// parameter, all @p data_parallel_degree processes fetch disjoint slices in
    /// parallel, so the aggregate bandwidth scales with the DP degree.
    std::vector<size_t> bandwidth_centric_partition(size_t bytes, size_t data_parallel_degree) const;

    /// Aggregate bandwidth available to @p data_parallel_degree processes each
    /// reading independently from @p tier.
    double aggregate_bandwidth(MemoryTier tier, size_t data_parallel_degree) const;

    /// Memory-centric tiling: splits an operator with @p rows rows of
    /// @p bytes_per_row into tiles that individually fit in @p tile_budget_bytes,
    /// so that a single layer larger than GPU memory can still be executed.
    static std::vector<ShardRange> memory_centric_tiles(size_t rows,
                                                        size_t bytes_per_row,
                                                        double tile_budget_bytes);

    /// Overlap-centric scheduling: a prefetched fetch is hidden behind compute,
    /// so the step costs max(compute, fetch) rather than their sum.
    static double overlapped_seconds(double compute_seconds, double transfer_seconds, bool overlap);

    /// Efficiency model of Section 4: e = ait * bw / (ait * bw + peak_throughput).
    static double efficiency(double arithmetic_intensity,
                             double bandwidth_bytes_per_second,
                             double peak_flops);

    /// Bandwidth needed to reach @p target_efficiency at the given arithmetic intensity.
    static double required_bandwidth(double arithmetic_intensity,
                                     double peak_flops,
                                     double target_efficiency);

private:
    const DeviceTier* find(MemoryTier tier) const;

    std::vector<DeviceTier> tiers_;
};

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_ZERO_INFINITY_H
