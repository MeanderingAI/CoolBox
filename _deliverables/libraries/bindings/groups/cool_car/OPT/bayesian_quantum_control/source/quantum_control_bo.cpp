#include "quantum_control_bo.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace {

double clamp_value(double value, double lower, double upper) {
    return std::max(lower, std::min(upper, value));
}

double sigmoid(double value) {
    if (value >= 40.0) return 1.0;
    if (value <= -40.0) return 0.0;
    return 1.0 / (1.0 + std::exp(-value));
}

double logit(double value) {
    const double clamped = clamp_value(value, 1e-9, 1.0 - 1e-9);
    return std::log(clamped / (1.0 - clamped));
}

double normalize_frequency(double omega, const opt::QuantumControlBayesianOptimizer::SearchBounds& bounds) {
    const double lower = (1.0 - bounds.initial_frequency_ratio) * bounds.frequency_center;
    const double upper = (1.0 + bounds.initial_frequency_ratio) * bounds.frequency_center;
    return clamp_value((omega - lower) / std::max(upper - lower, 1e-12), 0.0, 1.0);
}

double denormalize_frequency(double unit, const opt::QuantumControlBayesianOptimizer::SearchBounds& bounds) {
    const double lower = (1.0 - bounds.initial_frequency_ratio) * bounds.frequency_center;
    const double upper = (1.0 + bounds.initial_frequency_ratio) * bounds.frequency_center;
    return lower + clamp_value(unit, 0.0, 1.0) * (upper - lower);
}

std::vector<double> solve_linear_system(std::vector<std::vector<double>> matrix, std::vector<double> rhs) {
    const std::size_t n = rhs.size();
    for (std::size_t col = 0; col < n; ++col) {
        std::size_t pivot = col;
        for (std::size_t row = col + 1; row < n; ++row) {
            if (std::abs(matrix[row][col]) > std::abs(matrix[pivot][col])) pivot = row;
        }
        if (std::abs(matrix[pivot][col]) < 1e-12) continue;
        if (pivot != col) {
            std::swap(matrix[pivot], matrix[col]);
            std::swap(rhs[pivot], rhs[col]);
        }
        const double diagonal = matrix[col][col];
        for (std::size_t j = col; j < n; ++j) matrix[col][j] /= diagonal;
        rhs[col] /= diagonal;
        for (std::size_t row = 0; row < n; ++row) {
            if (row == col) continue;
            const double factor = matrix[row][col];
            for (std::size_t j = col; j < n; ++j) matrix[row][j] -= factor * matrix[col][j];
            rhs[row] -= factor * rhs[col];
        }
    }
    return rhs;
}

} // namespace

namespace opt {

QuantumControlBayesianOptimizer::QuantumControlBayesianOptimizer()
    : QuantumControlBayesianOptimizer(Config()) {}

QuantumControlBayesianOptimizer::QuantumControlBayesianOptimizer(const Config& config)
    : config_(config),
      current_bounds_(config.bounds),
      rng_(config.seed),
      best_score_(std::numeric_limits<double>::infinity()),
      best_probability_(-std::numeric_limits<double>::infinity()) {
    if (config_.initial_samples <= 0 || config_.iterations <= 0 || config_.acquisition_candidates <= 0) {
        throw std::invalid_argument("QuantumControlBayesianOptimizer: sample counts must be positive");
    }
    if (config_.bounds.frequency_center <= 0.0 || config_.bounds.initial_frequency_ratio <= 0.0) {
        throw std::invalid_argument("QuantumControlBayesianOptimizer: invalid frequency bounds");
    }
    if (config_.bounds.amplitude_min > config_.bounds.amplitude_max) {
        throw std::invalid_argument("QuantumControlBayesianOptimizer: invalid amplitude bounds");
    }
}

std::vector<double> QuantumControlBayesianOptimizer::sample_point() const {
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const double frequency_lower = (1.0 - current_bounds_.current_frequency_ratio) * current_bounds_.frequency_center;
    const double frequency_upper = (1.0 + current_bounds_.current_frequency_ratio) * current_bounds_.frequency_center;
    return {
        frequency_lower + unit(rng_) * (frequency_upper - frequency_lower),
        current_bounds_.amplitude_min + unit(rng_) * (current_bounds_.amplitude_max - current_bounds_.amplitude_min)
    };
}

std::vector<double> QuantumControlBayesianOptimizer::warp_frequency_point(const std::vector<double>& point) const {
    if (point.size() != 2 || best_solution_.empty()) return point;
    std::vector<double> warped = point;
    const double unit_frequency = normalize_frequency(point[0], config_.bounds);
    const double best_unit_frequency = normalize_frequency(best_solution_[0], config_.bounds);
    const double denominator = std::max(1.0 - best_probability_, 1e-6);
    warped[0] = denormalize_frequency(sigmoid((logit(unit_frequency) - logit(best_unit_frequency)) / denominator + logit(best_unit_frequency)), config_.bounds);
    return warped;
}

std::vector<double> QuantumControlBayesianOptimizer::unwarp_frequency_point(const std::vector<double>& point) const {
    if (point.size() != 2 || best_solution_.empty()) return point;
    std::vector<double> unwarped = point;
    const double unit_frequency = normalize_frequency(point[0], config_.bounds);
    const double best_unit_frequency = normalize_frequency(best_solution_[0], config_.bounds);
    const double denominator = std::max(1.0 - best_probability_, 1e-6);
    unwarped[0] = denormalize_frequency(sigmoid((logit(unit_frequency) - logit(best_unit_frequency)) * denominator + logit(best_unit_frequency)), config_.bounds);
    return unwarped;
}

double QuantumControlBayesianOptimizer::gaussian_process_ucb(
    const std::vector<Observation>& observations,
    const std::vector<double>& candidate) const {
    if (observations.empty()) return std::numeric_limits<double>::infinity();

    const std::size_t n = observations.size();
    std::vector<std::vector<double>> kernel(n, std::vector<double>(n, 0.0));
    std::vector<double> values(n, 0.0);
    std::vector<double> candidate_kernel(n, 0.0);

    auto transform = [this](const std::vector<double>& point) {
        return config_.policy == QuantumControlSearchPolicy::Warp ? warp_frequency_point(point) : point;
    };

    const std::vector<double> transformed_candidate = transform(candidate);
    for (std::size_t i = 0; i < n; ++i) {
        const std::vector<double> left = transform(observations[i].point);
        values[i] = observations[i].probability;
        for (std::size_t j = 0; j < n; ++j) {
            const std::vector<double> right = transform(observations[j].point);
            const double df = (left[0] - right[0]) / std::max(config_.length_scale_frequency, 1e-9);
            const double da = (left[1] - right[1]) / std::max(config_.length_scale_amplitude, 1e-9);
            kernel[i][j] = std::exp(-0.5 * (df * df + da * da));
        }
        kernel[i][i] += config_.observation_noise;
        const double cf = (transformed_candidate[0] - left[0]) / std::max(config_.length_scale_frequency, 1e-9);
        const double ca = (transformed_candidate[1] - left[1]) / std::max(config_.length_scale_amplitude, 1e-9);
        candidate_kernel[i] = std::exp(-0.5 * (cf * cf + ca * ca));
    }

    const std::vector<double> alpha = solve_linear_system(kernel, values);
    const double mean = std::inner_product(candidate_kernel.begin(), candidate_kernel.end(), alpha.begin(), 0.0);
    const std::vector<double> variance_weights = solve_linear_system(kernel, candidate_kernel);
    const double explained = std::inner_product(candidate_kernel.begin(), candidate_kernel.end(), variance_weights.begin(), 0.0);
    const double variance = std::max(0.0, 1.0 - explained);
    return mean + config_.exploration_beta * std::sqrt(variance);
}

std::vector<double> QuantumControlBayesianOptimizer::propose_candidate(const std::vector<Observation>& observations) const {
    std::vector<double> best_candidate = sample_point();
    double best_acquisition = -std::numeric_limits<double>::infinity();
    for (int i = 0; i < config_.acquisition_candidates; ++i) {
        std::vector<double> candidate = sample_point();
        if (config_.policy == QuantumControlSearchPolicy::Warp) {
            candidate = unwarp_frequency_point(candidate);
        }
        const double acquisition = gaussian_process_ucb(observations, candidate);
        if (acquisition > best_acquisition) {
            best_acquisition = acquisition;
            best_candidate = candidate;
        }
    }
    return best_candidate;
}

void QuantumControlBayesianOptimizer::update_search_policy(const std::vector<double>& point, double probability) {
    const bool new_best = probability >= best_probability_;
    if (new_best) {
        best_probability_ = probability;
        best_solution_ = point;
    }

    if (config_.policy == QuantumControlSearchPolicy::Standard || config_.policy == QuantumControlSearchPolicy::Warp) {
        current_bounds_.frequency_center = config_.bounds.frequency_center;
        current_bounds_.current_frequency_ratio = config_.bounds.initial_frequency_ratio;
        return;
    }

    current_bounds_.frequency_center = best_solution_.empty() ? config_.bounds.frequency_center : best_solution_[0];
    if (config_.policy == QuantumControlSearchPolicy::Crop || new_best) {
        current_bounds_.current_frequency_ratio = std::max(
            config_.crop_min_ratio,
            (1.0 - best_probability_) * (1.0 - best_probability_)) * config_.bounds.initial_frequency_ratio;
        return;
    }

    const double upper = config_.bounds.initial_frequency_ratio * std::max(1.0 - best_probability_, config_.crop_min_ratio);
    const double expanded = current_bounds_.current_frequency_ratio * (1.0 + config_.expansion_rate * std::max(0.0, best_probability_ - probability));
    current_bounds_.current_frequency_ratio = std::min(upper, std::max(config_.crop_min_ratio * config_.bounds.initial_frequency_ratio, expanded));
}

std::vector<double> QuantumControlBayesianOptimizer::optimize(
    const ObjectiveFunction& objective,
    const std::vector<double>& initial_state) {
    if (!initial_state.empty() && initial_state.size() != 2) {
        throw std::invalid_argument("QuantumControlBayesianOptimizer: initial_state must be empty or {frequency, amplitude}");
    }

    current_bounds_ = config_.bounds;
    rng_.seed(config_.seed);
    best_solution_.clear();
    best_score_ = std::numeric_limits<double>::infinity();
    best_probability_ = -std::numeric_limits<double>::infinity();

    std::vector<Observation> observations;
    auto evaluate = [&](const std::vector<double>& point) {
        const double score = objective(point);
        const double probability = clamp_value(-score, 0.0, 1.0);
        observations.push_back({point, probability});
        if (score < best_score_) {
            best_score_ = score;
        }
        update_search_policy(point, probability);
    };

    if (!initial_state.empty()) {
        evaluate(initial_state);
    }
    for (int i = static_cast<int>(observations.size()); i < config_.initial_samples; ++i) {
        evaluate(sample_point());
    }
    for (int i = 0; i < config_.iterations; ++i) {
        evaluate(propose_candidate(observations));
    }

    best_score_ = objective(best_solution_);
    return best_solution_;
}

const std::vector<double>& QuantumControlBayesianOptimizer::best_solution() const {
    return best_solution_;
}

double QuantumControlBayesianOptimizer::best_score() const {
    return best_score_;
}

QuantumControlBayesianOptimizer::SearchBounds QuantumControlBayesianOptimizer::current_bounds() const {
    return current_bounds_;
}

double QuantumControlBayesianOptimizer::best_probability() const {
    return best_probability_;
}

const char* to_string(QuantumControlSearchPolicy policy) {
    switch (policy) {
        case QuantumControlSearchPolicy::Standard: return "standard";
        case QuantumControlSearchPolicy::Crop: return "crop";
        case QuantumControlSearchPolicy::CropAndExpand: return "crop_and_expand";
        case QuantumControlSearchPolicy::Warp: return "warp";
        default: return "unknown";
    }
}

} // namespace opt