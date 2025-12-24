#include "ml/marked_point_process/marked_point_process.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>

using namespace Eigen;

// --- Constructor ---
MarkedPointProcess::MarkedPointProcess(
    int num_marks,
    double learning_rate,
    int max_iterations)
    : num_marks_(num_marks),
      learning_rate_(learning_rate),
      max_iterations_(max_iterations),
      beta_(1.0) {
    
    initialize_parameters();
}

// --- Initialize Parameters ---
void MarkedPointProcess::initialize_parameters() {
    // Initialize base intensity with small positive values
    mu_ = VectorXd::Constant(num_marks_, 0.1);
    
    // Initialize excitation matrix with small random positive values
    alpha_ = MatrixXd::Random(num_marks_, num_marks_);
    alpha_ = (alpha_.array() + 1.0) * 0.05; // Scale to [0, 0.1]
    
    // Ensure diagonal elements are slightly larger (self-excitation)
    for (int i = 0; i < num_marks_; ++i) {
        alpha_(i, i) += 0.05;
    }
    
    // Initialize decay rate
    beta_ = 1.0;
}

// --- Temporal Kernel (Exponential) ---
double MarkedPointProcess::kernel(double dt) const {
    if (dt <= 0) return 0.0;
    return beta_ * std::exp(-beta_ * dt);
}

// --- Kernel Integral ---
double MarkedPointProcess::kernel_integral(double t) const {
    if (t <= 0) return 0.0;
    return 1.0 - std::exp(-beta_ * t);
}

// --- Compute Intensity ---
double MarkedPointProcess::compute_intensity(
    double time,
    int mark,
    const std::vector<double>& history_times,
    const std::vector<int>& history_marks) const {
    
    // Start with base intensity
    double intensity = mu_(mark);
    
    // Add contributions from past events
    for (size_t i = 0; i < history_times.size(); ++i) {
        double dt = time - history_times[i];
        if (dt > 0) {
            int past_mark = history_marks[i];
            intensity += alpha_(past_mark, mark) * kernel(dt);
        }
    }
    
    return std::max(intensity, 1e-10); // Ensure positive intensity
}

// --- Predict Intensity ---
VectorXd MarkedPointProcess::predict_intensity(
    double time,
    const std::vector<double>& history_times,
    const std::vector<int>& history_marks) const {
    
    VectorXd intensities(num_marks_);
    
    for (int m = 0; m < num_marks_; ++m) {
        intensities(m) = compute_intensity(time, m, history_times, history_marks);
    }
    
    return intensities;
}

// --- Compute Compensator ---
VectorXd MarkedPointProcess::compute_compensator(
    const std::vector<double>& event_times,
    const std::vector<int>& event_marks,
    double time_horizon) const {
    
    VectorXd compensator = VectorXd::Zero(num_marks_);
    
    // Add base intensity contribution
    compensator = mu_ * time_horizon;
    
    // Add triggered intensity contributions
    for (size_t i = 0; i < event_times.size(); ++i) {
        double remaining_time = time_horizon - event_times[i];
        if (remaining_time > 0) {
            int mark_i = event_marks[i];
            for (int m = 0; m < num_marks_; ++m) {
                compensator(m) += alpha_(mark_i, m) * kernel_integral(remaining_time);
            }
        }
    }
    
    return compensator;
}

// --- Log-Likelihood ---
double MarkedPointProcess::log_likelihood(
    const std::vector<std::vector<double>>& event_times,
    const std::vector<std::vector<int>>& event_marks) const {
    
    double log_lik = 0.0;
    
    for (size_t seq = 0; seq < event_times.size(); ++seq) {
        const auto& times = event_times[seq];
        const auto& marks = event_marks[seq];
        
        if (times.empty()) continue;
        
        double time_horizon = times.back() + 1.0; // Observation window
        
        // Sum log intensities at event times
        for (size_t i = 0; i < times.size(); ++i) {
            std::vector<double> history_times(times.begin(), times.begin() + i);
            std::vector<int> history_marks(marks.begin(), marks.begin() + i);
            
            double intensity = compute_intensity(times[i], marks[i], 
                                                history_times, history_marks);
            log_lik += std::log(intensity);
        }
        
        // Subtract compensator (integrated intensity)
        VectorXd compensator = compute_compensator(times, marks, time_horizon);
        log_lik -= compensator.sum();
    }
    
    return log_lik;
}

// --- Update Parameters ---
void MarkedPointProcess::update_parameters(
    const std::vector<std::vector<double>>& event_times,
    const std::vector<std::vector<int>>& event_marks) {
    
    // Gradient accumulators
    VectorXd grad_mu = VectorXd::Zero(num_marks_);
    MatrixXd grad_alpha = MatrixXd::Zero(num_marks_, num_marks_);
    
    int total_events = 0;
    double total_time = 0.0;
    
    // Compute gradients for each sequence
    for (size_t seq = 0; seq < event_times.size(); ++seq) {
        const auto& times = event_times[seq];
        const auto& marks = event_marks[seq];
        
        if (times.empty()) continue;
        
        double time_horizon = times.back() + 1.0;
        total_time += time_horizon;
        total_events += times.size();
        
        // Gradient from log intensity terms
        for (size_t i = 0; i < times.size(); ++i) {
            std::vector<double> history_times(times.begin(), times.begin() + i);
            std::vector<int> history_marks(marks.begin(), marks.begin() + i);
            
            int mark_i = marks[i];
            double intensity = compute_intensity(times[i], mark_i, 
                                                history_times, history_marks);
            
            // Gradient w.r.t. mu
            grad_mu(mark_i) += 1.0 / intensity;
            
            // Gradient w.r.t. alpha
            for (size_t j = 0; j < history_times.size(); ++j) {
                double dt = times[i] - history_times[j];
                int mark_j = history_marks[j];
                grad_alpha(mark_j, mark_i) += kernel(dt) / intensity;
            }
        }
        
        // Gradient from compensator terms
        VectorXd compensator_grad_mu = VectorXd::Constant(num_marks_, time_horizon);
        grad_mu -= compensator_grad_mu;
        
        for (size_t i = 0; i < times.size(); ++i) {
            double remaining_time = time_horizon - times[i];
            if (remaining_time > 0) {
                int mark_i = marks[i];
                for (int m = 0; m < num_marks_; ++m) {
                    grad_alpha(mark_i, m) -= kernel_integral(remaining_time);
                }
            }
        }
    }
    
    // Update parameters with gradient ascent
    mu_ += learning_rate_ * grad_mu;
    alpha_ += learning_rate_ * grad_alpha;
    
    // Project to feasible region (non-negative parameters)
    mu_ = mu_.cwiseMax(1e-6);
    alpha_ = alpha_.cwiseMax(0.0);
}

// --- Fit (Training) ---
void MarkedPointProcess::fit(
    const std::vector<std::vector<double>>& event_times,
    const std::vector<std::vector<int>>& event_marks) {
    
    std::cout << "Training Marked Point Process with " 
              << event_times.size() << " sequences..." << std::endl;
    
    for (int iter = 0; iter < max_iterations_; ++iter) {
        // Compute log-likelihood before update
        double ll = log_likelihood(event_times, event_marks);
        
        // Update parameters
        update_parameters(event_times, event_marks);
        
        // Report progress
        if (iter % 100 == 0 || iter == max_iterations_ - 1) {
            std::cout << "Iteration " << std::setw(5) << iter 
                      << ", Log-Likelihood: " << std::fixed 
                      << std::setprecision(4) << ll << std::endl;
        }
    }
    
    std::cout << "Training complete." << std::endl;
}

// --- Generate Sequence ---
std::pair<std::vector<double>, std::vector<int>> 
MarkedPointProcess::generate_sequence(double time_horizon, int max_events) const {
    
    std::vector<double> times;
    std::vector<int> marks;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> uniform(0.0, 1.0);
    
    double t = 0.0;
    
    while (t < time_horizon && times.size() < static_cast<size_t>(max_events)) {
        // Compute current total intensity
        VectorXd intensities = predict_intensity(t, times, marks);
        double total_intensity = intensities.sum();
        
        if (total_intensity <= 0) break;
        
        // Sample next event time using thinning algorithm
        double dt = -std::log(uniform(gen)) / total_intensity;
        t += dt;
        
        if (t >= time_horizon) break;
        
        // Accept/reject event
        VectorXd new_intensities = predict_intensity(t, times, marks);
        double new_total_intensity = new_intensities.sum();
        
        if (uniform(gen) * total_intensity <= new_total_intensity) {
            // Accept event - sample mark type
            double u = uniform(gen) * new_total_intensity;
            double cumsum = 0.0;
            int selected_mark = 0;
            
            for (int m = 0; m < num_marks_; ++m) {
                cumsum += new_intensities(m);
                if (u <= cumsum) {
                    selected_mark = m;
                    break;
                }
            }
            
            times.push_back(t);
            marks.push_back(selected_mark);
        }
    }
    
    return {times, marks};
}

// --- Getters ---
VectorXd MarkedPointProcess::get_base_intensity() const {
    return mu_;
}

MatrixXd MarkedPointProcess::get_excitation_matrix() const {
    return alpha_;
}

double MarkedPointProcess::get_decay_rate() const {
    return beta_;
}

// --- Setters ---
void MarkedPointProcess::set_base_intensity(const VectorXd& base_intensity) {
    if (base_intensity.size() != num_marks_) {
        throw std::invalid_argument("Base intensity size must match num_marks");
    }
    mu_ = base_intensity;
}

void MarkedPointProcess::set_excitation_matrix(const MatrixXd& excitation) {
    if (excitation.rows() != num_marks_ || excitation.cols() != num_marks_) {
        throw std::invalid_argument("Excitation matrix must be num_marks x num_marks");
    }
    alpha_ = excitation;
}

void MarkedPointProcess::set_decay_rate(double decay) {
    if (decay <= 0) {
        throw std::invalid_argument("Decay rate must be positive");
    }
    beta_ = decay;
}
