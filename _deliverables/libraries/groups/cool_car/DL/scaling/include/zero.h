#ifndef ML_DEEP_LEARNING_SCALING_ZERO_H
#define ML_DEEP_LEARNING_SCALING_ZERO_H

#include "collective.h"
#include "tensor.h"

#include <cstddef>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// @brief ZeRO: Zero Redundancy Optimizer for data-parallel training.
///
/// Implements the three partitioning stages of
/// Rajbhandari et al., "ZeRO: Memory Optimizations Toward Training Trillion
/// Parameter Models", SC '20 (arXiv:1910.02054), citation key
/// @c rajbhandari_zero_2020.
enum class ZeroStage {
    kDisabled = 0,        ///< Classic data parallelism: every rank replicates everything.
    kOptimizerStates = 1, ///< P_os
    kGradients = 2,       ///< P_os+g
    kParameters = 3       ///< P_os+g+p
};

std::string to_string(ZeroStage stage);

/// Per-device memory breakdown, in bytes, for mixed-precision training.
///
/// Follows the model of Section 3 of the paper: 2 bytes/parameter for the fp16
/// weights, 2 bytes/parameter for the fp16 gradients, and @c optimizer_multiplier
/// bytes/parameter for the fp32 master weights plus Adam momentum and variance
/// (K = 12 for Adam).
struct ZeroMemoryEstimate {
    double parameter_bytes = 0.0;
    double gradient_bytes = 0.0;
    double optimizer_bytes = 0.0;
    double per_device_bytes = 0.0;
    /// Communication volume relative to plain data parallelism (1.0 for stages
    /// 1 and 2, 1.5 for stage 3).
    double communication_volume_factor = 1.0;
};

ZeroMemoryEstimate estimate_zero_memory(size_t num_parameters,
                                        ZeroStage stage,
                                        size_t world_size,
                                        double optimizer_multiplier = 12.0);

/// The largest model, in parameters, that fits in @p device_bytes per GPU.
size_t max_model_size(double device_bytes,
                      ZeroStage stage,
                      size_t world_size,
                      double optimizer_multiplier = 12.0);

/// Data-parallel engine that keeps only rank-local optimizer state.
///
/// A training step performs the reduce-scatter / local Adam update / all-gather
/// sequence described in Section 5 of the paper. Stage 1 and 2 rebuild the full
/// fp16 parameters after the update; stage 3 additionally requires an all-gather
/// during the forward pass, which is what raises its communication volume to 1.5x.
class ZeroDataParallelEngine {
public:
    struct Config {
        ZeroStage stage = ZeroStage::kGradients;
        double learning_rate = 1e-3;
        double beta1 = 0.9;
        double beta2 = 0.999;
        double epsilon = 1e-8;
        /// Averages the summed gradients over the world size, matching the
        /// semantics of torch DDP.
        bool average_gradients = true;
    };

    ZeroDataParallelEngine(size_t num_parameters, size_t world_size, Config config);

    /// Shard of the flattened parameter vector owned by @p rank.
    const ShardRange& shard_for(size_t rank) const;
    size_t owner_of(size_t parameter_index) const;

    /// Runs one optimizer step given each rank's local gradients and returns the
    /// updated, fully replicated parameters. @p parameters is updated in place.
    void step(Tensor& parameters, const std::vector<std::vector<double>>& per_rank_gradients);

    /// Forward-pass parameter all-gather, only issued for stage 3.
    void gather_parameters_for_forward(const Tensor& parameters);

    ZeroMemoryEstimate memory_estimate(double optimizer_multiplier = 12.0) const;

    /// Bytes each rank holds for optimizer state under the configured stage.
    size_t local_optimizer_state_elements() const;

    CollectiveGroup& collectives() { return collectives_; }
    const CollectiveGroup& collectives() const { return collectives_; }
    size_t step_count() const { return step_count_; }

private:
    size_t num_parameters_;
    size_t world_size_;
    Config config_;
    CollectiveGroup collectives_;
    std::vector<ShardRange> shards_;
    /// First and second Adam moments, held only for the locally owned shard.
    std::vector<std::vector<double>> momentum_;
    std::vector<std::vector<double>> variance_;
    size_t step_count_ = 0;
};

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_ZERO_H
