#include "marked_point_process.h"
#include <cmath>
#include <random>
#include <algorithm>

MarkedPointProcess::MarkedPointProcess(int num_marks, double learning_rate, int max_iterations)
    : num_marks_(num_marks), learning_rate_(learning_rate), max_iterations_(max_iterations),
      beta_(1.0) {
    initialize_parameters();
}

void MarkedPointProcess::initialize_parameters() {
    mu_ = Eigen::VectorXd::Constant(num_marks_, 0.1);
    alpha_ = Eigen::MatrixXd::Constant(num_marks_, num_marks_, 0.01);
    beta_ = 1.0;
}

double MarkedPointProcess::kernel(double dt) const {
    return beta_ * std::exp(-beta_ * dt);
}

double MarkedPointProcess::kernel_integral(double t) const {
    return 1.0 - std::exp(-beta_ * t);
}

double MarkedPointProcess::compute_intensity(double time, int mark,
    const std::vector<double>& history_times, const std::vector<int>& history_marks) const {
    double intensity = mu_(mark);
    for (size_t i = 0; i < history_times.size(); ++i) {
        if (history_times[i] >= time) break;
        double dt = time - history_times[i];
        intensity += alpha_(history_marks[i], mark) * kernel(dt);
    }
    return std::max(intensity, 1e-10);
}

void MarkedPointProcess::update_parameters(
    const std::vector<std::vector<double>>& event_times,
    const std::vector<std::vector<int>>& event_marks) {
    
    Eigen::VectorXd mu_grad = Eigen::VectorXd::Zero(num_marks_);
    Eigen::MatrixXd alpha_grad = Eigen::MatrixXd::Zero(num_marks_, num_marks_);
    
    for (size_t seq = 0; seq < event_times.size(); ++seq) {
        const auto& times = event_times[seq];
        const auto& marks = event_marks[seq];
        if (times.empty()) continue;
        double T = times.back();
        
        for (size_t i = 0; i < times.size(); ++i) {
            double lambda_i = compute_intensity(times[i], marks[i], times, marks);
            mu_grad(marks[i]) += 1.0 / lambda_i;
            
            for (size_t j = 0; j < i; ++j) {
                double dt = times[i] - times[j];
                alpha_grad(marks[j], marks[i]) += kernel(dt) / lambda_i;
            }
        }
        
        // Subtract compensator gradient
        for (int m = 0; m < num_marks_; ++m) {
            mu_grad(m) -= T;
            for (size_t i = 0; i < times.size(); ++i) {
                double remaining = T - times[i];
                alpha_grad(marks[i], m) -= kernel_integral(remaining);
            }
        }
    }
    
    double n_seq = static_cast<double>(event_times.size());
    mu_ += learning_rate_ * mu_grad / n_seq;
    alpha_ += learning_rate_ * alpha_grad / n_seq;
    
    // Ensure non-negative
    mu_ = mu_.cwiseMax(1e-10);
    alpha_ = alpha_.cwiseMax(0.0);
}

Eigen::VectorXd MarkedPointProcess::compute_compensator(
    const std::vector<double>& event_times, const std::vector<int>& event_marks,
    double time_horizon) const {
    Eigen::VectorXd comp = mu_ * time_horizon;
    for (size_t i = 0; i < event_times.size(); ++i) {
        double remaining = time_horizon - event_times[i];
        for (int m = 0; m < num_marks_; ++m) {
            comp(m) += alpha_(event_marks[i], m) * kernel_integral(remaining);
        }
    }
    return comp;
}

void MarkedPointProcess::fit(const std::vector<std::vector<double>>& event_times,
                              const std::vector<std::vector<int>>& event_marks) {
    for (int iter = 0; iter < max_iterations_; ++iter) {
        update_parameters(event_times, event_marks);
    }
}

Eigen::VectorXd MarkedPointProcess::predict_intensity(double time,
    const std::vector<double>& history_times, const std::vector<int>& history_marks) const {
    Eigen::VectorXd intensities(num_marks_);
    for (int m = 0; m < num_marks_; ++m) {
        intensities(m) = compute_intensity(time, m, history_times, history_marks);
    }
    return intensities;
}

std::pair<std::vector<double>, std::vector<int>> MarkedPointProcess::generate_sequence(
    double time_horizon, int max_events) const {
    std::vector<double> times;
    std::vector<int> marks;
    std::mt19937 gen(42);
    
    double t = 0.0;
    double lambda_max = mu_.sum() * 2.0;
    
    while (t < time_horizon && static_cast<int>(times.size()) < max_events) {
        std::exponential_distribution<double> exp_dist(lambda_max);
        t += exp_dist(gen);
        if (t >= time_horizon) break;
        
        Eigen::VectorXd intensities = predict_intensity(t, times, marks);
        double total_intensity = intensities.sum();
        
        std::uniform_real_distribution<double> uniform(0.0, 1.0);
        if (uniform(gen) < total_intensity / lambda_max) {
            // Accept: choose mark proportional to intensity
            double u = uniform(gen) * total_intensity;
            double cumsum = 0.0;
            int chosen_mark = 0;
            for (int m = 0; m < num_marks_; ++m) {
                cumsum += intensities(m);
                if (u <= cumsum) { chosen_mark = m; break; }
            }
            times.push_back(t);
            marks.push_back(chosen_mark);
            lambda_max = std::max(lambda_max, total_intensity * 1.5);
        }
    }
    return {times, marks};
}

double MarkedPointProcess::log_likelihood(const std::vector<std::vector<double>>& event_times,
                                           const std::vector<std::vector<int>>& event_marks) const {
    double ll = 0.0;
    for (size_t seq = 0; seq < event_times.size(); ++seq) {
        const auto& times = event_times[seq];
        const auto& mks = event_marks[seq];
        if (times.empty()) continue;
        double T = times.back();
        
        for (size_t i = 0; i < times.size(); ++i) {
            double lambda_i = compute_intensity(times[i], mks[i], times, mks);
            ll += std::log(lambda_i);
        }
        
        Eigen::VectorXd comp = compute_compensator(times, mks, T);
        ll -= comp.sum();
    }
    return ll;
}

Eigen::VectorXd MarkedPointProcess::get_base_intensity() const { return mu_; }
Eigen::MatrixXd MarkedPointProcess::get_excitation_matrix() const { return alpha_; }
double MarkedPointProcess::get_decay_rate() const { return beta_; }
void MarkedPointProcess::set_base_intensity(const Eigen::VectorXd& bi) { mu_ = bi; }
void MarkedPointProcess::set_excitation_matrix(const Eigen::MatrixXd& em) { alpha_ = em; }
void MarkedPointProcess::set_decay_rate(double d) { beta_ = d; }
