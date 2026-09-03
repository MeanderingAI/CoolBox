#include "one_bit_lamb.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

bool sign_bit(const OneBitPayload& payload, size_t index) {
    return (payload.signs[index / 8] >> (index % 8)) & 1u;
}

void set_sign_bit(OneBitPayload& payload, size_t index, bool positive) {
    if (positive) {
        payload.signs[index / 8] |= static_cast<uint8_t>(1u << (index % 8));
    }
}

} // namespace

OneBitPayload compress_one_bit(const std::vector<double>& values,
                               std::vector<double>& error,
                               size_t chunk_size) {
    if (chunk_size == 0) {
        throw std::invalid_argument("compress_one_bit: chunk_size must be > 0");
    }
    if (error.size() != values.size()) {
        error.assign(values.size(), 0.0);
    }

    OneBitPayload payload;
    payload.chunk_size = chunk_size;
    payload.element_count = values.size();
    payload.signs.assign((values.size() + 7) / 8, 0);

    for (size_t start = 0; start < values.size(); start += chunk_size) {
        const size_t end = std::min(start + chunk_size, values.size());

        double magnitude_sum = 0.0;
        for (size_t i = start; i < end; ++i) {
            magnitude_sum += std::abs(values[i] + error[i]);
        }
        const double scale = magnitude_sum / static_cast<double>(end - start);
        payload.scales.push_back(scale);

        for (size_t i = start; i < end; ++i) {
            const double compensated = values[i] + error[i];
            const bool positive = compensated >= 0.0;
            set_sign_bit(payload, i, positive);
            // Residual carried into the next step keeps the compression unbiased.
            error[i] = compensated - (positive ? scale : -scale);
        }
    }
    return payload;
}

std::vector<double> decompress_one_bit(const OneBitPayload& payload) {
    std::vector<double> values(payload.element_count, 0.0);
    for (size_t i = 0; i < payload.element_count; ++i) {
        const double scale = payload.scales[i / payload.chunk_size];
        values[i] = sign_bit(payload, i) ? scale : -scale;
    }
    return values;
}

double compression_ratio(const OneBitPayload& payload) {
    if (payload.element_count == 0) {
        return 1.0;
    }
    const double compressed = static_cast<double>(payload.signs.size()) +
                              static_cast<double>(payload.scales.size()) * sizeof(double);
    const double uncompressed = static_cast<double>(payload.element_count) * sizeof(float);
    return compressed / uncompressed;
}

std::vector<double> compressed_all_reduce(CollectiveGroup& group,
                                          const std::vector<std::vector<double>>& per_rank_values,
                                          std::vector<std::vector<double>>& per_rank_errors,
                                          size_t chunk_size) {
    if (per_rank_values.size() != group.world_size()) {
        throw std::invalid_argument("compressed_all_reduce: one buffer per rank is required");
    }
    if (per_rank_errors.size() != group.world_size()) {
        per_rank_errors.assign(group.world_size(), {});
    }

    std::vector<std::vector<double>> decompressed(group.world_size());
    for (size_t rank = 0; rank < group.world_size(); ++rank) {
        const OneBitPayload payload =
            compress_one_bit(per_rank_values[rank], per_rank_errors[rank], chunk_size);
        decompressed[rank] = decompress_one_bit(payload);
    }

    group.all_reduce(decompressed);
    return decompressed.front();
}

OneBitLamb::OneBitLamb() : OneBitLamb(Config()) {}

OneBitLamb::OneBitLamb(Config config) : config_(config) {
    if (config_.learning_rate <= 0.0) {
        throw std::invalid_argument("OneBitLamb: learning_rate must be > 0");
    }
    if (config_.min_coeff > config_.max_coeff) {
        throw std::invalid_argument("OneBitLamb: min_coeff must not exceed max_coeff");
    }
}

void OneBitLamb::ensure_initialized(size_t elements) {
    if (initialized_ && momentum_.size() == elements) {
        return;
    }
    momentum_.assign(elements, 0.0);
    variance_.assign(elements, 0.0);
    frozen_variance_.clear();
    error_.assign(elements, 0.0);
    initialized_ = true;
}

void OneBitLamb::step(Tensor& parameters, const Tensor& gradients) {
    if (parameters.size() != gradients.size()) {
        throw std::invalid_argument("OneBitLamb::step: parameter and gradient sizes must match");
    }
    ensure_initialized(parameters.size());
    ++step_count_;

    const bool refresh = config_.coeff_refresh_interval > 0 &&
                         step_count_ % config_.coeff_refresh_interval == 0;
    const bool warmup = step_count_ <= config_.warmup_steps;

    const double bias1 = 1.0 - std::pow(config_.beta1, static_cast<double>(step_count_));
    const double bias2 = 1.0 - std::pow(config_.beta2, static_cast<double>(step_count_));

    if (warmup) {
        // Phase 1: vanilla LAMB, communicating the momentum uncompressed.
        for (size_t i = 0; i < parameters.size(); ++i) {
            const double g = gradients.data()[i];
            momentum_[i] = config_.beta1 * momentum_[i] + (1.0 - config_.beta1) * g;
            variance_[i] = config_.beta2 * variance_[i] + (1.0 - config_.beta2) * g * g;
        }
        last_compression_ratio_ = 1.0;
        if (step_count_ == config_.warmup_steps) {
            frozen_variance_ = variance_;
        }
    } else {
        // Phase 2: only the momentum is exchanged, at 1 bit per element, and the
        // variance stays frozen at its end-of-warmup value.
        if (frozen_variance_.empty() || refresh) {
            frozen_variance_ = variance_;
        }
        std::vector<double> momentum_update(parameters.size());
        for (size_t i = 0; i < parameters.size(); ++i) {
            momentum_update[i] =
                config_.beta1 * momentum_[i] + (1.0 - config_.beta1) * gradients.data()[i];
        }
        const OneBitPayload payload = compress_one_bit(momentum_update, error_, config_.chunk_size);
        momentum_ = decompress_one_bit(payload);
        last_compression_ratio_ = compression_ratio(payload);
    }

    const std::vector<double>& adaptive = (warmup || frozen_variance_.empty())
        ? variance_
        : frozen_variance_;

    std::vector<double> update(parameters.size(), 0.0);
    double update_norm_sq = 0.0;
    double weight_norm_sq = 0.0;
    for (size_t i = 0; i < parameters.size(); ++i) {
        const double m_hat = momentum_[i] / bias1;
        const double v_hat = adaptive[i] / bias2;
        update[i] = m_hat / (std::sqrt(v_hat) + config_.epsilon) +
                    config_.weight_decay * parameters.data()[i];
        update_norm_sq += update[i] * update[i];
        weight_norm_sq += parameters.data()[i] * parameters.data()[i];
    }

    const double update_norm = std::sqrt(update_norm_sq);
    const double weight_norm = std::sqrt(weight_norm_sq);
    double trust_ratio = 1.0;
    if (weight_norm > 0.0 && update_norm > 0.0) {
        trust_ratio = std::clamp(weight_norm / update_norm, config_.min_coeff, config_.max_coeff);
    }
    last_trust_ratio_ = trust_ratio;

    for (size_t i = 0; i < parameters.size(); ++i) {
        parameters.data()[i] -= config_.learning_rate * trust_ratio * update[i];
    }
}

void OneBitLamb::reset() {
    momentum_.clear();
    variance_.clear();
    frozen_variance_.clear();
    error_.clear();
    step_count_ = 0;
    last_trust_ratio_ = 1.0;
    last_compression_ratio_ = 1.0;
    initialized_ = false;
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
