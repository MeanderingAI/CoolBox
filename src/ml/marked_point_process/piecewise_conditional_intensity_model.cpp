#include "ml/marked_point_process/piecewise_conditional_intensity_model.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <numeric>

using namespace Eigen;

// --- Constructor ---
PiecewiseConditionalIntensityModel::PiecewiseConditionalIntensityModel(
    int num_intervals,
    double learning_rate,
    int max_iterations)
    : num_intervals_(num_intervals),
      learning_rate_(learning_rate),
      max_iterations_(max_iterations) {
}

// --- Get Number of Parameters for Intensity Type ---
int PiecewiseConditionalIntensityModel::get_num_parameters(IntensityType type) const {
    switch (type) {
        case IntensityType::CONSTANT:
            return 1; // mu
        case IntensityType::LINEAR:
            return 2; // a, b
        case IntensityType::EXPONENTIAL:
            return 2; // a, b
        case IntensityType::HAWKES:
            return 3; // mu, alpha, beta
        case IntensityType::COX:
            return 5; // baseline + covariate weights (example)
        default:
            return 1;
    }
}

// --- Set Custom Intervals ---
void PiecewiseConditionalIntensityModel::set_intervals(
    const std::vector<TimeInterval>& intervals) {
    intervals_ = intervals;
    initialize_parameters();
}

// --- Create Uniform Intervals ---
void PiecewiseConditionalIntensityModel::create_uniform_intervals(
    double time_min,
    double time_max,
    IntensityType intensity_type) {
    
    intervals_.clear();
    double interval_width = (time_max - time_min) / num_intervals_;
    
    for (int i = 0; i < num_intervals_; ++i) {
        double start = time_min + i * interval_width;
        double end = (i == num_intervals_ - 1) ? time_max : time_min + (i + 1) * interval_width;
        intervals_.emplace_back(start, end, intensity_type);
    }
    
    initialize_parameters();
}

// --- Create Adaptive Intervals Based on Event Density ---
void PiecewiseConditionalIntensityModel::create_adaptive_intervals(
    const std::vector<double>& event_times,
    IntensityType intensity_type) {
    
    if (event_times.empty()) {
        throw std::runtime_error("Cannot create adaptive intervals with no events");
    }
    
    intervals_.clear();
    
    // Sort all events
    std::vector<double> sorted_times = event_times;
    std::sort(sorted_times.begin(), sorted_times.end());
    
    // Determine interval boundaries using quantiles
    int events_per_interval = std::max(1, static_cast<int>(sorted_times.size()) / num_intervals_);
    
    double prev_time = sorted_times.front();
    
    for (int i = 0; i < num_intervals_; ++i) {
        int idx = std::min(static_cast<int>((i + 1) * events_per_interval), 
                          static_cast<int>(sorted_times.size()) - 1);
        double end_time = (i == num_intervals_ - 1) ? sorted_times.back() : sorted_times[idx];
        
        intervals_.emplace_back(prev_time, end_time, intensity_type);
        prev_time = end_time;
    }
    
    initialize_parameters();
}

// --- Initialize Parameters ---
void PiecewiseConditionalIntensityModel::initialize_parameters() {
    interval_parameters_.clear();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.01, 0.1);
    
    for (size_t i = 0; i < intervals_.size(); ++i) {
        int num_params = get_num_parameters(intervals_[i].intensity_type);
        VectorXd params(num_params);
        
        // Initialize with small random positive values
        for (int j = 0; j < num_params; ++j) {
            params(j) = dis(gen);
        }
        
        // Special initialization for Hawkes: ensure stability (alpha < beta)
        if (intervals_[i].intensity_type == IntensityType::HAWKES && num_params >= 3) {
            params(0) = 0.1;  // mu
            params(1) = 0.3;  // alpha
            params(2) = 1.0;  // beta (decay > excitation)
        }
        
        interval_parameters_[i] = params;
        intervals_[i].parameters = params;
    }
}

// --- Find Interval for Given Time ---
int PiecewiseConditionalIntensityModel::find_interval(double time) const {
    for (size_t i = 0; i < intervals_.size(); ++i) {
        if (time >= intervals_[i].start_time && time < intervals_[i].end_time) {
            return i;
        }
        // Handle edge case for last interval
        if (i == intervals_.size() - 1 && time == intervals_[i].end_time) {
            return i;
        }
    }
    return -1;
}

// --- Compute Constant Intensity ---
double PiecewiseConditionalIntensityModel::compute_constant_intensity(
    double time,
    const VectorXd& params,
    const std::vector<double>& history_times) const {
    
    return std::max(params(0), 1e-10);
}

// --- Compute Linear Intensity ---
double PiecewiseConditionalIntensityModel::compute_linear_intensity(
    double time,
    const VectorXd& params,
    const std::vector<double>& history_times) const {
    
    double a = params(0);
    double b = params(1);
    return std::max(a + b * time, 1e-10);
}

// --- Compute Exponential Intensity ---
double PiecewiseConditionalIntensityModel::compute_exponential_intensity(
    double time,
    const VectorXd& params,
    const std::vector<double>& history_times) const {
    
    double a = params(0);
    double b = params(1);
    return std::max(a * std::exp(-b * time), 1e-10);
}

// --- Compute Hawkes Intensity ---
double PiecewiseConditionalIntensityModel::compute_hawkes_intensity(
    double time,
    const VectorXd& params,
    const std::vector<double>& history_times) const {
    
    double mu = params(0);      // Background rate
    double alpha = params(1);   // Excitation
    double beta = params(2);    // Decay
    
    double intensity = mu;
    
    // Add self-exciting term from history
    for (double t_i : history_times) {
        if (t_i < time) {
            double dt = time - t_i;
            intensity += alpha * std::exp(-beta * dt);
        }
    }
    
    return std::max(intensity, 1e-10);
}

// --- Compute Cox Intensity ---
double PiecewiseConditionalIntensityModel::compute_cox_intensity(
    double time,
    const VectorXd& params,
    const std::vector<double>& history_times,
    const VectorXd& covariates) const {
    
    // Cox model: lambda(t|covariates) = baseline * exp(beta' * X)
    double baseline = params(0);
    
    double linear_predictor = 0.0;
    int num_covariates = std::min(static_cast<int>(params.size()) - 1, 
                                   static_cast<int>(covariates.size()));
    
    for (int i = 0; i < num_covariates; ++i) {
        linear_predictor += params(i + 1) * covariates(i);
    }
    
    return std::max(baseline * std::exp(linear_predictor), 1e-10);
}

// --- Predict Intensity ---
double PiecewiseConditionalIntensityModel::predict_intensity(
    double time,
    const std::vector<double>& history_times) const {
    
    int interval_idx = find_interval(time);
    if (interval_idx < 0) {
        return 0.0; // Outside defined intervals
    }
    
    const auto& interval = intervals_[interval_idx];
    const auto& params = interval_parameters_.at(interval_idx);
    
    switch (interval.intensity_type) {
        case IntensityType::CONSTANT:
            return compute_constant_intensity(time, params, history_times);
        case IntensityType::LINEAR:
            return compute_linear_intensity(time, params, history_times);
        case IntensityType::EXPONENTIAL:
            return compute_exponential_intensity(time, params, history_times);
        case IntensityType::HAWKES:
            return compute_hawkes_intensity(time, params, history_times);
        default:
            return params(0); // Fallback to constant
    }
}

// --- Predict Intensity with Covariates ---
double PiecewiseConditionalIntensityModel::predict_intensity_with_covariates(
    double time,
    const std::vector<double>& history_times,
    const VectorXd& covariates) const {
    
    int interval_idx = find_interval(time);
    if (interval_idx < 0) {
        return 0.0;
    }
    
    const auto& interval = intervals_[interval_idx];
    const auto& params = interval_parameters_.at(interval_idx);
    
    if (interval.intensity_type == IntensityType::COX) {
        return compute_cox_intensity(time, params, history_times, covariates);
    } else {
        // Fallback to standard intensity
        return predict_intensity(time, history_times);
    }
}

// --- Compute Interval Compensator ---
double PiecewiseConditionalIntensityModel::compute_interval_compensator(
    int interval_idx,
    const std::vector<double>& event_times,
    const std::vector<double>& all_history) const {
    
    const auto& interval = intervals_[interval_idx];
    const auto& params = interval_parameters_.at(interval_idx);
    
    double start = interval.start_time;
    double end = interval.end_time;
    double duration = end - start;
    
    switch (interval.intensity_type) {
        case IntensityType::CONSTANT: {
            // Integral of constant: mu * duration
            return params(0) * duration;
        }
        
        case IntensityType::LINEAR: {
            // Integral of (a + b*t) from start to end
            double a = params(0);
            double b = params(1);
            return a * duration + 0.5 * b * (end * end - start * start);
        }
        
        case IntensityType::EXPONENTIAL: {
            // Integral of a*exp(-b*t)
            double a = params(0);
            double b = std::max(params(1), 1e-10);
            return (a / b) * (std::exp(-b * start) - std::exp(-b * end));
        }
        
        case IntensityType::HAWKES: {
            double mu = params(0);
            double alpha = params(1);
            double beta = std::max(params(2), 1e-10);
            
            double compensator = mu * duration;
            
            // Add contribution from all previous events (including from earlier intervals)
            for (double t_i : all_history) {
                if (t_i < end) {
                    double dt_start = std::max(0.0, start - t_i);
                    double dt_end = end - t_i;
                    compensator += (alpha / beta) * 
                        (std::exp(-beta * dt_start) - std::exp(-beta * dt_end));
                }
            }
            
            return compensator;
        }
        
        default:
            return params(0) * duration;
    }
}

// --- Compute Gradient ---
VectorXd PiecewiseConditionalIntensityModel::compute_gradient(
    int interval_idx,
    const std::vector<double>& event_times,
    const std::vector<double>& all_history) const {
    
    const auto& interval = intervals_[interval_idx];
    const auto& params = interval_parameters_.at(interval_idx);
    int num_params = params.size();
    
    VectorXd grad = VectorXd::Zero(num_params);
    
    // Gradient from log-likelihood sum
    for (double t : event_times) {
        double intensity = predict_intensity(t, all_history);
        
        if (interval.intensity_type == IntensityType::HAWKES && num_params >= 3) {
            // Gradient for Hawkes parameters
            grad(0) += 1.0 / intensity; // d/d_mu
            
            // d/d_alpha: sum of kernels
            double kernel_sum = 0.0;
            for (double t_i : all_history) {
                if (t_i < t) {
                    kernel_sum += std::exp(-params(2) * (t - t_i));
                }
            }
            grad(1) += kernel_sum / intensity;
            
            // d/d_beta: (partial derivative of kernel sum)
            double kernel_deriv = 0.0;
            for (double t_i : all_history) {
                if (t_i < t) {
                    double dt = t - t_i;
                    kernel_deriv += -dt * params(1) * std::exp(-params(2) * dt);
                }
            }
            grad(2) += kernel_deriv / intensity;
        } else if (interval.intensity_type == IntensityType::CONSTANT) {
            grad(0) += 1.0 / intensity;
        }
    }
    
    // Subtract compensator gradient
    double duration = interval.end_time - interval.start_time;
    
    if (interval.intensity_type == IntensityType::CONSTANT) {
        grad(0) -= duration;
    } else if (interval.intensity_type == IntensityType::HAWKES && num_params >= 3) {
        grad(0) -= duration;
        
        // Compensator gradients for alpha and beta
        double beta = std::max(params(2), 1e-10);
        for (double t_i : all_history) {
            if (t_i < interval.end_time) {
                double dt_start = std::max(0.0, interval.start_time - t_i);
                double dt_end = interval.end_time - t_i;
                grad(1) -= (1.0 / beta) * 
                    (std::exp(-beta * dt_start) - std::exp(-beta * dt_end));
            }
        }
    }
    
    return grad;
}

// --- Update Interval Parameters ---
void PiecewiseConditionalIntensityModel::update_interval_parameters(
    int interval_idx,
    const std::vector<double>& event_times,
    const std::vector<double>& all_history) {
    
    VectorXd grad = compute_gradient(interval_idx, event_times, all_history);
    
    // Gradient ascent
    interval_parameters_[interval_idx] += learning_rate_ * grad;
    
    // Project to feasible region (non-negative parameters)
    interval_parameters_[interval_idx] = 
        interval_parameters_[interval_idx].cwiseMax(1e-6);
    
    // For Hawkes, ensure stability: alpha < beta
    if (intervals_[interval_idx].intensity_type == IntensityType::HAWKES && 
        interval_parameters_[interval_idx].size() >= 3) {
        double alpha = interval_parameters_[interval_idx](1);
        double beta = interval_parameters_[interval_idx](2);
        if (alpha >= beta) {
            interval_parameters_[interval_idx](1) = beta * 0.9; // Keep alpha < beta
        }
    }
    
    intervals_[interval_idx].parameters = interval_parameters_[interval_idx];
}

// --- Fit (Training) ---
void PiecewiseConditionalIntensityModel::fit(
    const std::vector<std::vector<double>>& event_times) {
    
    std::cout << "Training Piecewise Conditional Intensity Model with " 
              << event_times.size() << " sequences and " 
              << intervals_.size() << " intervals..." << std::endl;
    
    for (int iter = 0; iter < max_iterations_; ++iter) {
        // Process each sequence
        for (const auto& sequence : event_times) {
            std::vector<double> all_history;
            
            // Process each interval
            for (size_t interval_idx = 0; interval_idx < intervals_.size(); ++interval_idx) {
                const auto& interval = intervals_[interval_idx];
                
                // Get events in this interval
                std::vector<double> interval_events;
                for (double t : sequence) {
                    if (t >= interval.start_time && t < interval.end_time) {
                        interval_events.push_back(t);
                    }
                }
                
                // Update parameters for this interval
                if (!interval_events.empty()) {
                    update_interval_parameters(interval_idx, interval_events, all_history);
                }
                
                // Add these events to history for next interval
                all_history.insert(all_history.end(), 
                                  interval_events.begin(), interval_events.end());
            }
        }
        
        // Report progress
        if (iter % 100 == 0 || iter == max_iterations_ - 1) {
            double ll = log_likelihood(event_times);
            std::cout << "Iteration " << std::setw(5) << iter 
                      << ", Log-Likelihood: " << std::fixed 
                      << std::setprecision(4) << ll << std::endl;
        }
    }
    
    std::cout << "Training complete." << std::endl;
}

// --- Log-Likelihood ---
double PiecewiseConditionalIntensityModel::log_likelihood(
    const std::vector<std::vector<double>>& event_times) const {
    
    double log_lik = 0.0;
    
    for (const auto& sequence : event_times) {
        std::vector<double> all_history;
        
        // Process each interval
        for (size_t interval_idx = 0; interval_idx < intervals_.size(); ++interval_idx) {
            // Get events in this interval
            std::vector<double> interval_events;
            for (double t : sequence) {
                if (t >= intervals_[interval_idx].start_time && 
                    t < intervals_[interval_idx].end_time) {
                    interval_events.push_back(t);
                }
            }
            
            // Log intensity sum for events in this interval
            for (double t : interval_events) {
                double intensity = predict_intensity(t, all_history);
                log_lik += std::log(intensity);
            }
            
            // Subtract compensator
            double compensator = compute_interval_compensator(
                interval_idx, interval_events, all_history);
            log_lik -= compensator;
            
            // Update history
            all_history.insert(all_history.end(), 
                              interval_events.begin(), interval_events.end());
        }
    }
    
    return log_lik;
}

// --- Generate Sequence ---
std::vector<double> PiecewiseConditionalIntensityModel::generate_sequence(
    double time_horizon,
    int max_events) const {
    
    std::vector<double> times;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> uniform(0.0, 1.0);
    
    double t = intervals_.front().start_time;
    
    while (t < time_horizon && times.size() < static_cast<size_t>(max_events)) {
        // Find current interval
        int interval_idx = find_interval(t);
        if (interval_idx < 0) break;
        
        // Compute upper bound on intensity
        double intensity_bound = predict_intensity(t, times) * 1.5;
        
        // Sample inter-event time using thinning
        double dt = -std::log(uniform(gen)) / intensity_bound;
        t += dt;
        
        if (t >= time_horizon) break;
        
        // Accept/reject
        double actual_intensity = predict_intensity(t, times);
        if (uniform(gen) * intensity_bound <= actual_intensity) {
            times.push_back(t);
        }
    }
    
    return times;
}

// --- Getters ---
std::vector<PiecewiseConditionalIntensityModel::TimeInterval> 
PiecewiseConditionalIntensityModel::get_intervals() const {
    return intervals_;
}

VectorXd PiecewiseConditionalIntensityModel::get_interval_parameters(int interval_index) const {
    auto it = interval_parameters_.find(interval_index);
    if (it != interval_parameters_.end()) {
        return it->second;
    }
    return VectorXd();
}

void PiecewiseConditionalIntensityModel::set_interval_parameters(
    int interval_index,
    const VectorXd& parameters) {
    if (interval_index >= 0 && interval_index < static_cast<int>(intervals_.size())) {
        interval_parameters_[interval_index] = parameters;
        intervals_[interval_index].parameters = parameters;
    }
}

// --- Get Expected Counts ---
VectorXd PiecewiseConditionalIntensityModel::get_expected_counts(
    const std::vector<std::vector<double>>& event_times) const {
    
    VectorXd expected_counts = VectorXd::Zero(intervals_.size());
    
    for (const auto& sequence : event_times) {
        std::vector<double> all_history;
        
        for (size_t idx = 0; idx < intervals_.size(); ++idx) {
            std::vector<double> interval_events;
            for (double t : sequence) {
                if (t >= intervals_[idx].start_time && t < intervals_[idx].end_time) {
                    interval_events.push_back(t);
                }
            }
            
            double compensator = compute_interval_compensator(idx, interval_events, all_history);
            expected_counts(idx) += compensator;
            
            all_history.insert(all_history.end(), interval_events.begin(), interval_events.end());
        }
    }
    
    return expected_counts;
}

// --- Compute Information Criteria ---
std::pair<double, double> PiecewiseConditionalIntensityModel::compute_information_criteria(
    const std::vector<std::vector<double>>& event_times) const {
    
    double ll = log_likelihood(event_times);
    
    // Count total parameters
    int total_params = 0;
    for (const auto& interval : intervals_) {
        total_params += get_num_parameters(interval.intensity_type);
    }
    
    // Count total events
    int n = 0;
    for (const auto& seq : event_times) {
        n += seq.size();
    }
    
    // AIC = -2*LL + 2*k
    double aic = -2.0 * ll + 2.0 * total_params;
    
    // BIC = -2*LL + k*log(n)
    double bic = -2.0 * ll + total_params * std::log(std::max(n, 1));
    
    return {aic, bic};
}

// --- Fit with Covariates ---
void PiecewiseConditionalIntensityModel::fit_with_covariates(
    const std::vector<std::vector<double>>& event_times,
    const std::vector<MatrixXd>& covariates) {
    
    // For now, simplified implementation - just call standard fit
    // Full covariate support would require extensive modifications
    std::cout << "Covariate-based fitting not fully implemented, using standard fit." << std::endl;
    fit(event_times);
}
