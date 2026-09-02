#include "piecewise_conditional_intensity_model.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <numeric>
#include <limits>

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
    const std::vector<mytrix::DenseMatrix>& covariates) {
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

double PiecewiseConditionalIntensityModel::compute_tip17_visual_support(
    int semantic_label,
    double time,
    const std::vector<Tip17VisualObservation>& observations,
    const Tip17InferenceConfig& config) const {
    const double bandwidth = std::max(config.visual_bandwidth, 1e-6);
    double support = 0.0;
    for (const auto& observation : observations) {
        const auto mapped_label = config.visual_word_to_semantic_label.find(observation.visual_word);
        if (mapped_label != config.visual_word_to_semantic_label.end() && mapped_label->second != semantic_label) {
            continue;
        }
        const double distance = std::abs(observation.time - time);
        support += std::max(0.0, observation.confidence) * std::exp(-distance / bandwidth);
    }
    return support;
}

double PiecewiseConditionalIntensityModel::compute_tip17_candidate_probability(
    int semantic_label,
    double time,
    const std::vector<double>& history_times,
    const std::vector<Tip17VisualObservation>& observations,
    const Tip17InferenceConfig& config) const {
    const double model_rate = std::max(predict_intensity(time, history_times), config.base_semantic_rate);
    const double support = compute_tip17_visual_support(semantic_label, time, observations, config);
    const double logit = std::log(model_rate) + config.visual_weight * support;
    if (logit >= 40.0) return 1.0;
    if (logit <= -40.0) return 0.0;
    return 1.0 / (1.0 + std::exp(-logit));
}

PiecewiseConditionalIntensityModel::Tip17InferenceResult
PiecewiseConditionalIntensityModel::infer_tip17_video_events(
    const std::vector<Tip17VisualObservation>& observations,
    const Tip17InferenceConfig& config) const {
    if (config.semantic_label_count <= 0) {
        throw std::invalid_argument("semantic_label_count must be positive");
    }

    Tip17InferenceResult result;
    result.observed_events = observations;
    result.semantic_label_scores.assign(static_cast<size_t>(config.semantic_label_count), 0.0);
    if (observations.empty()) return result;

    std::sort(result.observed_events.begin(), result.observed_events.end(),
        [](const Tip17VisualObservation& left, const Tip17VisualObservation& right) {
            return left.time < right.time;
        });

    const double first_time = result.observed_events.front().time;
    const double last_time = result.observed_events.back().time;
    const double duration = std::min(
        std::max(config.default_event_duration, config.min_event_duration),
        std::max(config.min_event_duration, config.max_event_duration));
    const int retained_samples = std::max(1, config.sample_count - config.burn_in);
    std::mt19937 generator(config.random_seed);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);

    struct Candidate {
        double time;
        int semantic_label;
        bool from_visual_evidence;
    };

    std::vector<Candidate> candidates;
    for (const auto& observation : result.observed_events) {
        const auto mapped_label = config.visual_word_to_semantic_label.find(observation.visual_word);
        if (mapped_label != config.visual_word_to_semantic_label.end()) {
            if (mapped_label->second >= 0 && mapped_label->second < config.semantic_label_count) {
                candidates.push_back({observation.time, mapped_label->second, true});
            }
        } else {
            for (int label = 0; label < config.semantic_label_count; ++label) {
                candidates.push_back({observation.time, label, true});
            }
        }
    }

    const double horizon = std::max(last_time - first_time, config.min_event_duration);
    const double lambda_star = std::max(config.base_semantic_rate, 1e-6) * std::max(config.auxiliary_rate_multiplier, 1.0);
    std::poisson_distribution<int> virtual_count_dist(lambda_star * horizon);
    for (int label = 0; label < config.semantic_label_count; ++label) {
        const int virtual_count = virtual_count_dist(generator);
        result.virtual_event_count += virtual_count;
        for (int i = 0; i < virtual_count; ++i) {
            candidates.push_back({first_time + uniform(generator) * horizon, label, false});
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        if (left.time == right.time) return left.semantic_label < right.semantic_label;
        return left.time < right.time;
    });

    std::vector<int> acceptance_counts(candidates.size(), 0);
    std::vector<double> history_times;
    for (int sample = 0; sample < config.sample_count; ++sample) {
        history_times.clear();
        for (size_t i = 0; i < candidates.size(); ++i) {
            const double probability = compute_tip17_candidate_probability(
                candidates[i].semantic_label,
                candidates[i].time,
                history_times,
                result.observed_events,
                config);
            const bool keep = candidates[i].from_visual_evidence
                ? probability >= uniform(generator)
                : probability >= uniform(generator) && probability > 0.5;
            if (keep) {
                history_times.push_back(candidates[i].time);
                if (sample >= config.burn_in) {
                    ++acceptance_counts[i];
                    result.semantic_label_scores[static_cast<size_t>(candidates[i].semantic_label)] += 1.0;
                }
            }
        }
    }

    for (size_t i = 0; i < candidates.size(); ++i) {
        const double posterior = static_cast<double>(acceptance_counts[i]) / static_cast<double>(retained_samples);
        if (posterior >= config.posterior_threshold) {
            const double start_time = candidates[i].time;
            result.semantic_events.push_back({
                candidates[i].semantic_label,
                start_time,
                std::min(start_time + duration, last_time + duration),
                posterior
            });
        }
    }

    const double normalizer = static_cast<double>(retained_samples);
    for (double& score : result.semantic_label_scores) {
        score /= normalizer;
    }

    return result;
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
