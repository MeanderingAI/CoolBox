#include "hidden_markov_model.h"
#include <cmath>
#include <limits>
#include <algorithm>

double HMM::log_sum_exp(double log_a, double log_b) {
    if (log_a == -std::numeric_limits<double>::infinity()) return log_b;
    if (log_b == -std::numeric_limits<double>::infinity()) return log_a;
    double max_val = std::max(log_a, log_b);
    return max_val + std::log(std::exp(log_a - max_val) + std::exp(log_b - max_val));
}

HMM::HMM(int states, int observations)
    : num_states(states), num_observations(observations), gen(42) {
    initial_probabilities = Eigen::VectorXd::Ones(num_states) / num_states;
    transition_matrix = Eigen::MatrixXd::Ones(num_states, num_states) / num_states;
    emission_matrix = Eigen::MatrixXd::Ones(num_states, num_observations) / num_observations;
}

void HMM::set_initial_probabilities(const Eigen::VectorXd& pi) { initial_probabilities = pi; }
void HMM::set_transition_matrix(const Eigen::MatrixXd& A) { transition_matrix = A; }
void HMM::set_emission_matrix(const Eigen::MatrixXd& B) { emission_matrix = B; }
Eigen::VectorXd HMM::get_initial_probabilities() const { return initial_probabilities; }
Eigen::MatrixXd HMM::get_transition_matrix() const { return transition_matrix; }
Eigen::MatrixXd HMM::get_emission_matrix() const { return emission_matrix; }

Eigen::MatrixXd HMM::forward_pass(const std::vector<int>& observations) const {
    int T = static_cast<int>(observations.size());
    Eigen::MatrixXd alpha(num_states, T);
    // Init
    for (int s = 0; s < num_states; ++s)
        alpha(s, 0) = std::log(initial_probabilities(s)) + std::log(emission_matrix(s, observations[0]));
    // Recurse
    for (int t = 1; t < T; ++t) {
        for (int s = 0; s < num_states; ++s) {
            double log_sum = -std::numeric_limits<double>::infinity();
            for (int prev = 0; prev < num_states; ++prev)
                log_sum = log_sum_exp(log_sum, alpha(prev, t-1) + std::log(transition_matrix(prev, s)));
            alpha(s, t) = log_sum + std::log(emission_matrix(s, observations[t]));
        }
    }
    return alpha;
}

Eigen::MatrixXd HMM::backward_pass(const std::vector<int>& observations) const {
    int T = static_cast<int>(observations.size());
    Eigen::MatrixXd beta(num_states, T);
    // Init
    for (int s = 0; s < num_states; ++s)
        beta(s, T-1) = 0.0; // log(1)
    // Recurse
    for (int t = T - 2; t >= 0; --t) {
        for (int s = 0; s < num_states; ++s) {
            double log_sum = -std::numeric_limits<double>::infinity();
            for (int next = 0; next < num_states; ++next)
                log_sum = log_sum_exp(log_sum, std::log(transition_matrix(s, next)) + std::log(emission_matrix(next, observations[t+1])) + beta(next, t+1));
            beta(s, t) = log_sum;
        }
    }
    return beta;
}

double HMM::log_likelihood(const std::vector<int>& observations) const {
    auto alpha = forward_pass(observations);
    int T = static_cast<int>(observations.size());
    double ll = -std::numeric_limits<double>::infinity();
    for (int s = 0; s < num_states; ++s)
        ll = log_sum_exp(ll, alpha(s, T-1));
    return ll;
}

std::vector<int> HMM::get_most_likely_states(const std::vector<int>& observations) const {
    int T = static_cast<int>(observations.size());
    Eigen::MatrixXd delta(num_states, T);
    Eigen::MatrixXi psi(num_states, T);
    // Init
    for (int s = 0; s < num_states; ++s) {
        delta(s, 0) = std::log(initial_probabilities(s)) + std::log(emission_matrix(s, observations[0]));
        psi(s, 0) = 0;
    }
    // Recurse
    for (int t = 1; t < T; ++t) {
        for (int s = 0; s < num_states; ++s) {
            double best = -std::numeric_limits<double>::infinity();
            int best_prev = 0;
            for (int prev = 0; prev < num_states; ++prev) {
                double v = delta(prev, t-1) + std::log(transition_matrix(prev, s));
                if (v > best) { best = v; best_prev = prev; }
            }
            delta(s, t) = best + std::log(emission_matrix(s, observations[t]));
            psi(s, t) = best_prev;
        }
    }
    // Backtrack
    std::vector<int> path(T);
    double best = -std::numeric_limits<double>::infinity();
    for (int s = 0; s < num_states; ++s) {
        if (delta(s, T-1) > best) { best = delta(s, T-1); path[T-1] = s; }
    }
    for (int t = T - 2; t >= 0; --t)
        path[t] = psi(path[t+1], t+1);
    return path;
}

void HMM::train(const std::vector<std::vector<int>>& observation_sequences, int max_iterations, double tolerance, double smoothing_factor, unsigned int seed) {
    gen.seed(seed);
    // Random initialization
    std::uniform_real_distribution<double> dist(0.1, 1.0);
    for (int i = 0; i < num_states; ++i) {
        initial_probabilities(i) = dist(gen);
        for (int j = 0; j < num_states; ++j) transition_matrix(i, j) = dist(gen);
        for (int o = 0; o < num_observations; ++o) emission_matrix(i, o) = dist(gen);
    }
    initial_probabilities /= initial_probabilities.sum();
    for (int i = 0; i < num_states; ++i) {
        transition_matrix.row(i) /= transition_matrix.row(i).sum();
        emission_matrix.row(i) /= emission_matrix.row(i).sum();
    }

    double prev_ll = -std::numeric_limits<double>::infinity();
    for (int iter = 0; iter < max_iterations; ++iter) {
        Eigen::VectorXd new_pi = Eigen::VectorXd::Zero(num_states) + Eigen::VectorXd::Constant(num_states, smoothing_factor);
        Eigen::MatrixXd new_A = Eigen::MatrixXd::Zero(num_states, num_states) + Eigen::MatrixXd::Constant(num_states, num_states, smoothing_factor);
        Eigen::MatrixXd new_B = Eigen::MatrixXd::Zero(num_states, num_observations) + Eigen::MatrixXd::Constant(num_states, num_observations, smoothing_factor);
        double total_ll = 0.0;

        for (const auto& obs : observation_sequences) {
            if (obs.empty()) continue;
            int T = static_cast<int>(obs.size());
            auto alpha = forward_pass(obs);
            auto beta = backward_pass(obs);
            double ll = -std::numeric_limits<double>::infinity();
            for (int s = 0; s < num_states; ++s) ll = log_sum_exp(ll, alpha(s, T-1));
            total_ll += ll;

            // Gamma and Xi
            for (int t = 0; t < T; ++t) {
                for (int s = 0; s < num_states; ++s) {
                    double gamma_s = std::exp(alpha(s, t) + beta(s, t) - ll);
                    if (t == 0) new_pi(s) += gamma_s;
                    new_B(s, obs[t]) += gamma_s;
                    if (t < T - 1) {
                        for (int j = 0; j < num_states; ++j) {
                            double xi = std::exp(alpha(s, t) + std::log(transition_matrix(s, j)) +
                                        std::log(emission_matrix(j, obs[t+1])) + beta(j, t+1) - ll);
                            new_A(s, j) += xi;
                        }
                    }
                }
            }
        }

        // Normalize
        initial_probabilities = new_pi / new_pi.sum();
        for (int i = 0; i < num_states; ++i) {
            double row_sum_A = new_A.row(i).sum();
            if (row_sum_A > 0) transition_matrix.row(i) = new_A.row(i) / row_sum_A;
            double row_sum_B = new_B.row(i).sum();
            if (row_sum_B > 0) emission_matrix.row(i) = new_B.row(i) / row_sum_B;
        }

        if (std::abs(total_ll - prev_ll) < tolerance) break;
        prev_ll = total_ll;
    }
}
