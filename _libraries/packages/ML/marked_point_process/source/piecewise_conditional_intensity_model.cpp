#include "piecewise_conditional_intensity_model.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <numeric>

PiecewiseConditionalIntensityModel::PiecewiseConditionalIntensityModel(
        int num_intervals, double learning_rate, int max_iterations)
        : num_intervals_(num_intervals), learning_rate_(learning_rate),
            max_iterations_(max_iterations) {
        for (int i = 0; i < num_intervals; ++i) {
                intervals_.emplace_back(static_cast<double>(i), static_cast<double>(i + 1), IntensityType::CONSTANT);
        }
}

void PiecewiseConditionalIntensityModel::set_intervals(const std::vector<TimeInterval>& intervals) {
    intervals_ = intervals;
    num_intervals_ = static_cast<int>(intervals.size());
    initialize_parameters();
}

void PiecewiseConditionalIntensityModel::create_uniform_intervals(
    double time_min, double time_max, IntensityType type) {
    intervals_.clear();
    double step = (time_max - time_min) / num_intervals_;
    for (int i = 0; i < num_intervals_; ++i) {
        intervals_.emplace_back(time_min + i * step, time_min + (i + 1) * step, type);
    }
    initialize_parameters();
}

void PiecewiseConditionalIntensityModel::create_adaptive_intervals(
    const std::vector<double>& event_times, IntensityType type) {
    if (event_times.empty()) return;
    intervals_.clear();
    double t_min = *std::min_element(event_times.begin(), event_times.end());
    double t_max = *std::max_element(event_times.begin(), event_times.end());
    create_uniform_intervals(t_min, t_max + 0.01, type);
}

int PiecewiseConditionalIntensityModel::find_interval(double time) const {
    for (int i = 0; i < static_cast<int>(intervals_.size()); ++i) {
        if (time >= intervals_[i].start_time && time < intervals_[i].end_time) return i;
    }
    return static_cast<int>(intervals_.size()) - 1;
}

int PiecewiseConditionalIntensityModel::get_num_parameters(IntensityType type) const {
    switch (type) {
        case IntensityType::CONSTANT: return 1;
        case IntensityType::LINEAR: return 2;
        case IntensityType::EXPONENTIAL: return 2;
        case IntensityType::HAWKES: return 3;
        case IntensityType::COX: return 2;
        default: return 1;
    }
}

void PiecewiseConditionalIntensityModel::initialize_parameters() {
    for (auto& interval : intervals_) {
        int np = get_num_parameters(interval.intensity_type);
        interval.parameters = std::vector<double>(np, 0.1);
    }
}

double PiecewiseConditionalIntensityModel::compute_constant_intensity(
    double /*time*/, const std::vector<double>& params, const std::vector<double>& /*history*/) const {
    return std::max(params[0], 1e-10);
}

double PiecewiseConditionalIntensityModel::compute_linear_intensity(
    double time, const std::vector<double>& params, const std::vector<double>& /*history*/) const {
    return std::max(params[0] + params[1] * time, 1e-10);
}

double PiecewiseConditionalIntensityModel::compute_exponential_intensity(
    double time, const std::vector<double>& params, const std::vector<double>& /*history*/) const {
    return std::max(params[0] * std::exp(params[1] * time), 1e-10);
}

double PiecewiseConditionalIntensityModel::compute_hawkes_intensity(
    double time, const std::vector<double>& params, const std::vector<double>& history) const {
    double mu = params[0];
    double alpha = params[1];
    double beta = params[2];
    double intensity = mu;
    for (double t : history) {
        if (t >= time) break;
        intensity += alpha * beta * std::exp(-beta * (time - t));
    }
    return std::max(intensity, 1e-10);
}

double PiecewiseConditionalIntensityModel::compute_cox_intensity(
    double time, const std::vector<double>& params, const std::vector<double>& /*history*/,
    const std::vector<double>& covariates) const {
    double base = params[0];
    // TODO: Replace Eigen dot product with std::inner_product
    double dot = 0.0;
    for (size_t i = 0; i < covariates.size(); ++i) dot += covariates[i] * params[1];
    double result = base * std::exp(dot);
    return std::max(result, 1e-10);
}

double PiecewiseConditionalIntensityModel::predict_intensity(
    double time, const std::vector<double>& history_times) const {
    int idx = find_interval(time);
    if (idx < 0 || idx >= static_cast<int>(intervals_.size())) return 0.0;
    const auto& interval = intervals_[idx];
    switch (interval.intensity_type) {
        case IntensityType::CONSTANT: return compute_constant_intensity(time, interval.parameters, history_times);
        case IntensityType::LINEAR: return compute_linear_intensity(time, interval.parameters, history_times);
        case IntensityType::EXPONENTIAL: return compute_exponential_intensity(time, interval.parameters, history_times);
        case IntensityType::HAWKES: return compute_hawkes_intensity(time, interval.parameters, history_times);
        default: return compute_constant_intensity(time, interval.parameters, history_times);
    }
}

double PiecewiseConditionalIntensityModel::predict_intensity_with_covariates(
    double time, const std::vector<double>& history_times, const std::vector<double>& covariates) const {
    int idx = find_interval(time);
    if (idx < 0) return 0.0;
    const auto& interval = intervals_[idx];
    if (interval.intensity_type == IntensityType::COX) {
        return compute_cox_intensity(time, interval.parameters, history_times, covariates);
    }
    return predict_intensity(time, history_times);
}

double PiecewiseConditionalIntensityModel::compute_interval_compensator(
    int interval_idx, const std::vector<double>& event_times,
    const std::vector<double>& all_history) const {
    if (interval_idx < 0 || interval_idx >= static_cast<int>(intervals_.size())) return 0.0;
    const auto& interval = intervals_[interval_idx];
    double dt = interval.end_time - interval.start_time;
    double lambda = predict_intensity((interval.start_time + interval.end_time) / 2.0, all_history);
    return lambda * dt;
}

std::vector<double> PiecewiseConditionalIntensityModel::compute_gradient(
    int interval_idx, const std::vector<double>& event_times,
    const std::vector<double>& all_history) const {
    const auto& interval = intervals_[interval_idx];
    int np = interval.parameters.size();
    std::vector<double> grad(np, 0.01); // Simplified stub
    return grad;
}

void PiecewiseConditionalIntensityModel::update_interval_parameters(
    int interval_idx, const std::vector<double>& event_times,
    const std::vector<double>& all_history) {
    std::vector<double> grad = compute_gradient(interval_idx, event_times, all_history);
    for (size_t i = 0; i < grad.size(); ++i) {
        intervals_[interval_idx].parameters[i] += learning_rate_ * grad[i];
        intervals_[interval_idx].parameters[i] = std::max(intervals_[interval_idx].parameters[i], 1e-10);
    }
}

void PiecewiseConditionalIntensityModel::fit(const std::vector<std::vector<double>>& event_times) {
    if (intervals_.empty()) {
        // Auto-create intervals
        double t_min = 1e10, t_max = -1e10;
        for (const auto& seq : event_times) {
            for (double t : seq) { t_min = std::min(t_min, t); t_max = std::max(t_max, t); }
        }
        create_uniform_intervals(t_min, t_max + 0.01, IntensityType::CONSTANT);
    }
    
    for (int iter = 0; iter < max_iterations_; ++iter) {
        for (const auto& seq : event_times) {
            for (int idx = 0; idx < static_cast<int>(intervals_.size()); ++idx) {
                update_interval_parameters(idx, seq, seq);
            }
        }
    }
}

void PiecewiseConditionalIntensityModel::fit_with_covariates(
    const std::vector<std::vector<double>>& event_times,
    const std::vector<matrix::DenseMatrix>& covariates) {
    fit(event_times); // Simplified
}

std::vector<double> PiecewiseConditionalIntensityModel::generate_sequence(
    double time_horizon, int max_events) const {
    std::vector<double> times;
    std::mt19937 gen(42);
    double t = 0.0;
    
    while (t < time_horizon && static_cast<int>(times.size()) < max_events) {
        double lambda = predict_intensity(t, times);
        std::exponential_distribution<double> exp_dist(std::max(lambda, 0.01));
        t += exp_dist(gen);
        if (t < time_horizon) times.push_back(t);
    }
    return times;
}

double PiecewiseConditionalIntensityModel::log_likelihood(
    const std::vector<std::vector<double>>& event_times) const {
    double ll = 0.0;
    for (const auto& seq : event_times) {
        if (seq.empty()) continue;
        for (size_t i = 0; i < seq.size(); ++i) {
            ll += std::log(predict_intensity(seq[i], seq));
        }
        // Subtract compensator
        for (int idx = 0; idx < static_cast<int>(intervals_.size()); ++idx) {
            ll -= compute_interval_compensator(idx, seq, seq);
        }
    }
    return ll;
}

std::vector<PiecewiseConditionalIntensityModel::TimeInterval>
PiecewiseConditionalIntensityModel::get_intervals() const { return intervals_; }

std::vector<double> PiecewiseConditionalIntensityModel::get_interval_parameters(int idx) const {
    return intervals_[idx].parameters;
}

void PiecewiseConditionalIntensityModel::set_interval_parameters(int idx, const std::vector<double>& params) {
    intervals_[idx].parameters = params;
}

std::vector<double> PiecewiseConditionalIntensityModel::get_expected_counts(
    const std::vector<std::vector<double>>& event_times) const {
    std::vector<double> counts(intervals_.size(), 0.0);
    for (const auto& seq : event_times) {
        for (double t : seq) {
            int idx = find_interval(t);
            if (idx >= 0 && idx < static_cast<int>(counts.size())) counts[idx] += 1.0;
        }
    }
    for (auto& c : counts) c /= static_cast<double>(event_times.size());
    return counts;
}

std::pair<double, double> PiecewiseConditionalIntensityModel::compute_information_criteria(
    const std::vector<std::vector<double>>& event_times) const {
    double ll = log_likelihood(event_times);
    int k = 0;
    for (const auto& interval : intervals_) k += static_cast<int>(interval.parameters.size());
    int n = 0;
    for (const auto& seq : event_times) n += static_cast<int>(seq.size());
    double aic = -2.0 * ll + 2.0 * static_cast<double>(k);
    double bic = -2.0 * ll + static_cast<double>(k) * std::log(static_cast<double>(n));
    return {aic, bic};
}
