// Implementation of MarkedPointProcess
#include "../headers/marked_point_process.h"
#include <vector>
#include <random>
#include <numeric>
#include <algorithm>

ml::MarkedPointProcess::MarkedPointProcess(int num_marks, double learning_rate, int max_iterations)
    : num_marks_(num_marks), learning_rate_(learning_rate), max_iterations_(max_iterations), beta_(1.0) {
    initialize_parameters();
}

void ml::MarkedPointProcess::initialize_parameters() {
    mu_ = std::vector<double>(num_marks_, 0.1);
    alpha_ = matrix::DenseMatrix(num_marks_, num_marks_);
    for (int i = 0; i < num_marks_; ++i)
        for (int j = 0; j < num_marks_; ++j)
            alpha_.at(i, j) = 0.01;
    beta_ = 1.0;
}

double ml::MarkedPointProcess::kernel(double dt) const {
    // NOTE: This is a stub. Implement kernel logic as needed.
    return 0.0;
}

void ml::MarkedPointProcess::update_parameters(
    const std::vector<std::vector<double>>& event_times,
    const std::vector<std::vector<int>>& event_marks) {
    // NOTE: This is a stub. Implement gradient logic for std::vector/matrix::DenseMatrix as needed.
}

void ml::MarkedPointProcess::fit(const std::vector<std::vector<double>>& event_times,
                              const std::vector<std::vector<int>>& event_marks) {
    for (int iter = 0; iter < max_iterations_; ++iter) {
        update_parameters(event_times, event_marks);
    }
}

std::vector<double> ml::MarkedPointProcess::predict_intensity(double time,
    const std::vector<double>& history_times, const std::vector<int>& history_marks) const {
    std::vector<double> intensities(num_marks_, 0.0);
    for (int m = 0; m < num_marks_; ++m) {
        intensities[m] = compute_intensity(time, m, history_times, history_marks);
    }
    return intensities;
}

std::pair<std::vector<double>, std::vector<int>> ml::MarkedPointProcess::generate_sequence(
    double time_horizon, int max_events) const {
    std::vector<double> times;
    std::vector<int> marks;
    std::mt19937 gen(42);
    
    double t = 0.0;
    double lambda_max = std::accumulate(mu_.begin(), mu_.end(), 0.0) * 2.0;
    
    while (t < time_horizon && static_cast<int>(times.size()) < max_events) {
        std::exponential_distribution<double> exp_dist(lambda_max);
        t += exp_dist(gen);
        if (t >= time_horizon) break;
        
        std::vector<double> intensities = predict_intensity(t, times, marks);
        double total_intensity = std::accumulate(intensities.begin(), intensities.end(), 0.0);
        
        std::uniform_real_distribution<double> uniform(0.0, 1.0);
        if (uniform(gen) < total_intensity / lambda_max) {
            // Accept: choose mark proportional to intensity
            double u = uniform(gen) * total_intensity;
            double cumsum = 0.0;
            int chosen_mark = 0;
            for (int m = 0; m < num_marks_; ++m) {
                cumsum += intensities[m];
                if (u <= cumsum) { chosen_mark = m; break; }
            }
            times.push_back(t);
            marks.push_back(chosen_mark);
            lambda_max = std::max(lambda_max, total_intensity * 1.5);
        }
    }
    return {times, marks};
}

double ml::MarkedPointProcess::log_likelihood(const std::vector<std::vector<double>>& event_times,
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
        
        std::vector<double> comp = compute_compensator(times, mks, T);
        ll -= std::accumulate(comp.begin(), comp.end(), 0.0);
    }
    return ll;
}

std::vector<double> ml::MarkedPointProcess::get_base_intensity() const { return mu_; }
matrix::DenseMatrix ml::MarkedPointProcess::get_excitation_matrix() const { return alpha_; }
double ml::MarkedPointProcess::get_decay_rate() const { return beta_; }
void ml::MarkedPointProcess::set_base_intensity(const std::vector<double>& bi) { mu_ = bi; }
void ml::MarkedPointProcess::set_excitation_matrix(const matrix::DenseMatrix& em) { alpha_ = em; }
void ml::MarkedPointProcess::set_decay_rate(double d) { beta_ = d; }
double ml::MarkedPointProcess::kernel_integral(double t) const {
    // NOTE: This is a stub. Implement kernel integral logic as needed.
    return 0.0;
}

double ml::MarkedPointProcess::compute_intensity(
    double time,
    int mark,
    const std::vector<double>& history_times,
    const std::vector<int>& history_marks
) const {
    double intensity = (mark >= 0 && mark < num_marks_) ? mu_[mark] : 0.0;
    for (size_t i = 0; i < history_times.size(); ++i) {
        double dt = time - history_times[i];
        if (dt <= 0.0) continue;
        int hm = history_marks[i];
        if (hm < 0 || hm >= num_marks_ || mark < 0 || mark >= num_marks_) continue;
        intensity += alpha_.at(mark, hm) * std::exp(-beta_ * dt);
    }
    return std::max(intensity, 1e-10);
}

std::vector<double> ml::MarkedPointProcess::compute_compensator(
    const std::vector<double>& event_times,
    const std::vector<int>& event_marks,
    double time_horizon
) const {
    std::vector<double> comp(num_marks_, 0.0);
    for (int m = 0; m < num_marks_; ++m) {
        comp[m] = mu_[m] * time_horizon;
        for (size_t i = 0; i < event_times.size(); ++i) {
            int hm = event_marks[i];
            if (hm < 0 || hm >= num_marks_) continue;
            double dt = time_horizon - event_times[i];
            if (dt <= 0.0) continue;
            comp[m] += alpha_.at(m, hm) / beta_ * (1.0 - std::exp(-beta_ * dt));
        }
    }
    return comp;
}

