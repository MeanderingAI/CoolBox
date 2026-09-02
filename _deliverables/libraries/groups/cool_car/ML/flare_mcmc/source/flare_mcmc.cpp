#include "flare_mcmc.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ml {

namespace {

double log_add_exp(double log_left, double log_right) {
    if (std::isinf(log_left) && log_left < 0.0) return log_right;
    if (std::isinf(log_right) && log_right < 0.0) return log_left;

    const double high = std::max(log_left, log_right);
    const double low = std::min(log_left, log_right);
    return high + std::log1p(std::exp(low - high));
}

} // namespace

double FlareLayerStats::acceptance_rate() const {
    if (proposals == 0) {
        return 0.0;
    }
    return static_cast<double>(accepted) / static_cast<double>(proposals);
}

FlareMcmcSampler::FlareMcmcSampler(std::vector<FlareLayer> layers, FlareConfig config)
    : layers_(std::move(layers))
    , config_(config)
    , stats_(layers_.size())
    , rng_(config.seed) {
    if (layers_.empty()) {
        throw std::invalid_argument("FLARE MCMC requires at least one fidelity layer");
    }
    if (config_.inner_steps == 0) {
        throw std::invalid_argument("FLARE MCMC inner_steps must be positive");
    }
    if (config_.proposal_stddev <= 0.0) {
        throw std::invalid_argument("FLARE MCMC proposal_stddev must be positive");
    }
    if (config_.tuning_learning_rate < 0.0) {
        throw std::invalid_argument("FLARE MCMC tuning learning rate must be non-negative");
    }

    for (std::size_t i = 0; i < layers_.size(); ++i) {
        if (!layers_[i].log_density) {
            throw std::invalid_argument("FLARE MCMC layer is missing a log density");
        }
        layers_[i].tuning_omega = std::max(config_.min_tuning_omega, layers_[i].tuning_omega);
    }
    layers_.front().tuning_omega = 0.0;
}

FlareRunResult FlareMcmcSampler::sample(const FlareState& initial_state, std::size_t sample_count) {
    validate_state(initial_state);
    stats_.assign(layers_.size(), FlareLayerStats{});

    FlareRunResult result;
    result.samples = run_chain(0, initial_state, sample_count);
    result.layer_stats = stats_;
    result.tuning_omegas.reserve(layers_.size());
    for (const auto& layer : layers_) {
        result.tuning_omegas.push_back(layer.tuning_omega);
    }
    return result;
}

double FlareMcmcSampler::effective_log_density(std::size_t layer_index, const FlareState& state) const {
    const double log_density = raw_log_density(layer_index, state);
    const double omega = layers_.at(layer_index).tuning_omega;
    if (!config_.enable_layer_tuning || layer_index == 0 || omega <= 0.0) {
        return log_density;
    }
    return log_add_exp(log_density, std::log(omega));
}

double FlareMcmcSampler::acceptance_log_ratio(std::size_t layer_index,
                                              const FlareState& current,
                                              const FlareState& proposal) const {
    if (layer_index >= layers_.size()) {
        throw std::out_of_range("FLARE MCMC layer index is out of range");
    }

    const double current_target = effective_log_density(layer_index, current);
    const double proposal_target = effective_log_density(layer_index, proposal);
    if (layer_index + 1u >= layers_.size()) {
        return proposal_target - current_target;
    }

    const double current_proposal = effective_log_density(layer_index + 1u, current);
    const double proposal_proposal = effective_log_density(layer_index + 1u, proposal);
    return proposal_target - current_target + current_proposal - proposal_proposal;
}

std::vector<FlareState> FlareMcmcSampler::run_chain(std::size_t layer_index,
                                                    const FlareState& initial_state,
                                                    std::size_t sample_count) {
    std::vector<FlareState> samples;
    samples.reserve(sample_count);

    FlareState current = initial_state;
    for (std::size_t i = 0; i < sample_count; ++i) {
        ChainStep step = step_chain(layer_index, current);
        current = std::move(step.state);
        samples.push_back(current);
    }
    return samples;
}

FlareMcmcSampler::ChainStep FlareMcmcSampler::step_chain(std::size_t layer_index, const FlareState& current) {
    FlareState proposal;
    if (layer_index + 1u == layers_.size()) {
        proposal = gaussian_proposal(current);
    } else {
        std::vector<FlareState> nested = run_chain(layer_index + 1u, current, config_.inner_steps);
        proposal = nested.back();
        if (config_.enable_layer_tuning) {
            update_tuning(layer_index + 1u, current, proposal);
        }
    }

    const double log_ratio = acceptance_log_ratio(layer_index, current, proposal);
    const bool accepted = accept(log_ratio);
    stats_[layer_index].proposals += 1;
    if (accepted) {
        stats_[layer_index].accepted += 1;
    }
    return ChainStep{accepted ? proposal : current, log_ratio, accepted};
}

FlareState FlareMcmcSampler::gaussian_proposal(const FlareState& current) {
    std::normal_distribution<double> normal(0.0, config_.proposal_stddev);
    FlareState proposal = current;
    for (double& value : proposal) {
        value += normal(rng_);
    }
    return proposal;
}

bool FlareMcmcSampler::accept(double log_acceptance_ratio) {
    if (log_acceptance_ratio >= 0.0) {
        return true;
    }
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    return std::log(uniform(rng_)) < log_acceptance_ratio;
}

void FlareMcmcSampler::update_tuning(std::size_t approximating_layer,
                                     const FlareState& start_state,
                                     const FlareState& end_state) {
    if (approximating_layer == 0 || approximating_layer >= layers_.size()) {
        return;
    }

    const double omega = layers_[approximating_layer].tuning_omega;
    const double start_density = raw_density(approximating_layer, start_state);
    const double end_density = raw_density(approximating_layer, end_state);
    const double epsilon = std::numeric_limits<double>::min();
    const double gradient = 1.0 / std::max(start_density + omega, epsilon) -
                            1.0 / std::max(end_density + omega, epsilon);

    layers_[approximating_layer].tuning_omega = std::max(
        config_.min_tuning_omega,
        omega + config_.tuning_learning_rate * gradient
    );
}

double FlareMcmcSampler::raw_log_density(std::size_t layer_index, const FlareState& state) const {
    validate_state(state);
    const double value = layers_.at(layer_index).log_density(state);
    stats_[layer_index].log_density_evaluations += 1;
    if (std::isnan(value)) {
        throw std::runtime_error("FLARE MCMC log density returned NaN");
    }
    return value;
}

double FlareMcmcSampler::raw_density(std::size_t layer_index, const FlareState& state) const {
    const double log_value = raw_log_density(layer_index, state);
    if (log_value >= std::log(std::numeric_limits<double>::max())) {
        return std::numeric_limits<double>::max();
    }
    return std::exp(log_value);
}

void FlareMcmcSampler::validate_state(const FlareState& state) const {
    if (state.empty()) {
        throw std::invalid_argument("FLARE MCMC state must not be empty");
    }
    for (double value : state) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("FLARE MCMC state contains a non-finite value");
        }
    }
}

} // namespace ml