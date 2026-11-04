#include "zero.h"

#include <cmath>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

constexpr double kFp16BytesPerParameter = 2.0;

} // namespace

std::string to_string(ZeroStage stage) {
    switch (stage) {
        case ZeroStage::kDisabled: return "disabled";
        case ZeroStage::kOptimizerStates: return "P_os";
        case ZeroStage::kGradients: return "P_os+g";
        case ZeroStage::kParameters: return "P_os+g+p";
    }
    return "unknown";
}

ZeroMemoryEstimate estimate_zero_memory(size_t num_parameters,
                                        ZeroStage stage,
                                        size_t world_size,
                                        double optimizer_multiplier) {
    if (world_size == 0) {
        throw std::invalid_argument("estimate_zero_memory: world_size must be > 0");
    }

    const double psi = static_cast<double>(num_parameters);
    const double n = static_cast<double>(world_size);

    ZeroMemoryEstimate estimate;
    estimate.parameter_bytes = kFp16BytesPerParameter * psi;
    estimate.gradient_bytes = kFp16BytesPerParameter * psi;
    estimate.optimizer_bytes = optimizer_multiplier * psi;
    estimate.communication_volume_factor = 1.0;

    switch (stage) {
        case ZeroStage::kDisabled:
            break;
        case ZeroStage::kOptimizerStates:
            estimate.optimizer_bytes /= n;
            break;
        case ZeroStage::kGradients:
            estimate.optimizer_bytes /= n;
            estimate.gradient_bytes /= n;
            break;
        case ZeroStage::kParameters:
            estimate.optimizer_bytes /= n;
            estimate.gradient_bytes /= n;
            estimate.parameter_bytes /= n;
            estimate.communication_volume_factor = 1.5;
            break;
    }

    estimate.per_device_bytes =
        estimate.parameter_bytes + estimate.gradient_bytes + estimate.optimizer_bytes;
    return estimate;
}

size_t max_model_size(double device_bytes,
                      ZeroStage stage,
                      size_t world_size,
                      double optimizer_multiplier) {
    const ZeroMemoryEstimate unit = estimate_zero_memory(1, stage, world_size, optimizer_multiplier);
    if (unit.per_device_bytes <= 0.0 || device_bytes <= 0.0) {
        return 0;
    }
    return static_cast<size_t>(device_bytes / unit.per_device_bytes);
}

ZeroDataParallelEngine::ZeroDataParallelEngine(size_t num_parameters,
                                               size_t world_size,
                                               Config config)
    : num_parameters_(num_parameters),
      world_size_(world_size),
      config_(config),
      collectives_(world_size),
      shards_(partition_evenly(num_parameters, world_size)) {
    const bool partitioned = config_.stage != ZeroStage::kDisabled;
    momentum_.resize(world_size_);
    variance_.resize(world_size_);
    for (size_t rank = 0; rank < world_size_; ++rank) {
        const size_t owned = partitioned ? shards_[rank].count : num_parameters_;
        momentum_[rank].assign(owned, 0.0);
        variance_[rank].assign(owned, 0.0);
    }
}

const ShardRange& ZeroDataParallelEngine::shard_for(size_t rank) const {
    if (rank >= world_size_) {
        throw std::out_of_range("ZeroDataParallelEngine: rank is out of range");
    }
    return shards_[rank];
}

size_t ZeroDataParallelEngine::owner_of(size_t parameter_index) const {
    for (size_t rank = 0; rank < world_size_; ++rank) {
        if (shards_[rank].contains(parameter_index)) {
            return rank;
        }
    }
    throw std::out_of_range("ZeroDataParallelEngine: parameter index is out of range");
}

size_t ZeroDataParallelEngine::local_optimizer_state_elements() const {
    // Two Adam moments plus the fp32 master copy.
    const size_t owned = config_.stage == ZeroStage::kDisabled
        ? num_parameters_
        : shards_.front().count;
    return 3 * owned;
}

ZeroMemoryEstimate ZeroDataParallelEngine::memory_estimate(double optimizer_multiplier) const {
    return estimate_zero_memory(num_parameters_, config_.stage, world_size_, optimizer_multiplier);
}

void ZeroDataParallelEngine::gather_parameters_for_forward(const Tensor& parameters) {
    if (config_.stage != ZeroStage::kParameters) {
        return; // Stages 0-2 keep the full fp16 weights resident on every rank.
    }
    std::vector<std::vector<double>> shards(world_size_);
    for (size_t rank = 0; rank < world_size_; ++rank) {
        const ShardRange& range = shards_[rank];
        shards[rank].assign(parameters.data().begin() + static_cast<std::ptrdiff_t>(range.offset),
                            parameters.data().begin() + static_cast<std::ptrdiff_t>(range.end()));
    }
    std::vector<std::vector<double>> gathered;
    collectives_.all_gather(shards, gathered);
}

void ZeroDataParallelEngine::step(Tensor& parameters,
                                  const std::vector<std::vector<double>>& per_rank_gradients) {
    if (parameters.size() != num_parameters_) {
        throw std::invalid_argument("ZeroDataParallelEngine::step: parameter count mismatch");
    }
    if (per_rank_gradients.size() != world_size_) {
        throw std::invalid_argument("ZeroDataParallelEngine::step: one gradient buffer per rank is required");
    }

    ++step_count_;
    const double scale = config_.average_gradients ? 1.0 / static_cast<double>(world_size_) : 1.0;
    const double bias1 = 1.0 - std::pow(config_.beta1, static_cast<double>(step_count_));
    const double bias2 = 1.0 - std::pow(config_.beta2, static_cast<double>(step_count_));

    if (config_.stage == ZeroStage::kDisabled) {
        std::vector<std::vector<double>> buffers = per_rank_gradients;
        collectives_.all_reduce(buffers);
        for (size_t i = 0; i < num_parameters_; ++i) {
            const double gradient = buffers[0][i] * scale;
            double& m = momentum_[0][i];
            double& v = variance_[0][i];
            m = config_.beta1 * m + (1.0 - config_.beta1) * gradient;
            v = config_.beta2 * v + (1.0 - config_.beta2) * gradient * gradient;
            parameters.data()[i] -=
                config_.learning_rate * (m / bias1) / (std::sqrt(v / bias2) + config_.epsilon);
        }
        // Replicated optimizer state: every rank keeps an identical copy.
        for (size_t rank = 1; rank < world_size_; ++rank) {
            momentum_[rank] = momentum_[0];
            variance_[rank] = variance_[0];
        }
        return;
    }

    // Stage 1-3: each rank reduces into, and updates, only its own shard.
    std::vector<std::vector<double>> owned_gradients;
    collectives_.reduce_scatter(per_rank_gradients, owned_gradients);

    std::vector<std::vector<double>> updated_shards(world_size_);
    for (size_t rank = 0; rank < world_size_; ++rank) {
        const ShardRange& range = shards_[rank];
        updated_shards[rank].resize(range.count);
        for (size_t local = 0; local < range.count; ++local) {
            const double gradient = owned_gradients[rank][local] * scale;
            double& m = momentum_[rank][local];
            double& v = variance_[rank][local];
            m = config_.beta1 * m + (1.0 - config_.beta1) * gradient;
            v = config_.beta2 * v + (1.0 - config_.beta2) * gradient * gradient;

            const double updated = parameters.data()[range.offset + local] -
                config_.learning_rate * (m / bias1) / (std::sqrt(v / bias2) + config_.epsilon);
            updated_shards[rank][local] = updated;
        }
    }

    std::vector<std::vector<double>> gathered;
    collectives_.all_gather(updated_shards, gathered);
    parameters.data() = gathered.front();
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
