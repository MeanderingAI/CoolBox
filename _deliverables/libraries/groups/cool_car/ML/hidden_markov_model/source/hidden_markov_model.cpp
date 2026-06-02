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

double HMM::log_sum_exp(const std::vector<double>& vals) {
    if (vals.empty()) return -std::numeric_limits<double>::infinity();
    double max_val = *std::max_element(vals.begin(), vals.end());
    if (max_val == -std::numeric_limits<double>::infinity()) return max_val;
    double sum = 0.0;
    for (double v : vals) sum += std::exp(v - max_val);
    return max_val + std::log(sum);
}

HMM::HMM(int states, int observations)
    : num_states(states), num_observations(observations), gen(42) {
    initial_probabilities = std::vector<double>(num_states, 1.0 / num_states);
    transition_matrix = mytrix::DenseMatrix(num_states, num_states);
    emission_matrix = mytrix::DenseMatrix(num_states, num_observations);
    for (int i = 0; i < num_states; ++i) {
        for (int j = 0; j < num_states; ++j) {
            transition_matrix.at(i, j) = 1.0 / num_states;
        }
        for (int j = 0; j < num_observations; ++j) {
            emission_matrix.at(i, j) = 1.0 / num_observations;
        }
    }
}

void HMM::set_initial_probabilities(const std::vector<double>& pi) { initial_probabilities = pi; }
void HMM::set_transition_matrix(const mytrix::DenseMatrix& A) { transition_matrix = A; }
void HMM::set_emission_matrix(const mytrix::DenseMatrix& B) { emission_matrix = B; }
std::vector<double> HMM::get_initial_probabilities() const { return initial_probabilities; }
mytrix::DenseMatrix HMM::get_transition_matrix() const { return transition_matrix; }
mytrix::DenseMatrix HMM::get_emission_matrix() const { return emission_matrix; }

mytrix::DenseMatrix HMM::forward_pass(const std::vector<int>& observations) const {
    int T = static_cast<int>(observations.size());
    mytrix::DenseMatrix alpha(num_states, T);
    // Init
    for (int s = 0; s < num_states; ++s)
        alpha.at(s, 0) = std::log(initial_probabilities[s]) + std::log(emission_matrix.at(s, observations[0]));
    // Recurse
    for (int t = 1; t < T; ++t) {
        for (int s = 0; s < num_states; ++s) {
            double log_sum = -std::numeric_limits<double>::infinity();
            for (int prev = 0; prev < num_states; ++prev)
                log_sum = log_sum_exp(log_sum, alpha.at(prev, t-1) + std::log(transition_matrix.at(prev, s)));
            alpha.at(s, t) = log_sum + std::log(emission_matrix.at(s, observations[t]));
        }
    }
    return alpha;
}

mytrix::DenseMatrix HMM::backward_pass(const std::vector<int>& observations) const {
    int T = static_cast<int>(observations.size());
    mytrix::DenseMatrix beta(num_states, T);
    // Init
    for (int s = 0; s < num_states; ++s)
        beta.at(s, T-1) = 0.0; // log(1)
    // Recurse
    for (int t = T - 2; t >= 0; --t) {
        for (int s = 0; s < num_states; ++s) {
            double log_sum = -std::numeric_limits<double>::infinity();
            for (int next = 0; next < num_states; ++next)
                log_sum = HMM::log_sum_exp(log_sum, std::log(transition_matrix.at(s, next)) + std::log(emission_matrix.at(next, observations[t+1])) + beta.at(next, t+1));
            beta.at(s, t) = log_sum;
        }
    }
    return beta;
}

double HMM::log_likelihood(const std::vector<int>& observations) const {
    auto alpha = forward_pass(observations);
    int T = static_cast<int>(observations.size());
    std::vector<double> finals(num_states);
    for (int s = 0; s < num_states; ++s)
        finals[s] = alpha.at(s, T-1);
    return HMM::log_sum_exp(finals);
}

std::vector<int> HMM::get_most_likely_states(const std::vector<int>& observations) const {
    int T = static_cast<int>(observations.size());
    mytrix::DenseMatrix delta(num_states, T);
    mytrix::DenseMatrix psi(num_states, T); // store as double, cast to int
    // Init
    for (int s = 0; s < num_states; ++s) {
        delta.at(s, 0) = std::log(initial_probabilities[s]) + std::log(emission_matrix.at(s, observations[0]));
        psi.at(s, 0) = 0;
    }
    // Recurse
    for (int t = 1; t < T; ++t) {
        for (int s = 0; s < num_states; ++s) {
            double best = -std::numeric_limits<double>::infinity();
            int best_prev = 0;
            for (int prev = 0; prev < num_states; ++prev) {
                double v = delta.at(prev, t-1) + std::log(transition_matrix.at(prev, s));
                if (v > best) { best = v; best_prev = prev; }
            }
            delta.at(s, t) = best + std::log(emission_matrix.at(s, observations[t]));
            psi.at(s, t) = best_prev;
        }
    }
    // Backtrack
    std::vector<int> path(T);
    double best = -std::numeric_limits<double>::infinity();
    for (int s = 0; s < num_states; ++s) {
        if (delta.at(s, T-1) > best) { best = delta.at(s, T-1); path[T-1] = s; }
    }
    for (int t = T - 2; t >= 0; --t)
        path[t] = static_cast<int>(psi.at(path[t+1], t+1));
    return path;
}

void HMM::train(const std::vector<std::vector<int>>& observation_sequences, int max_iterations, double tolerance, double smoothing_factor, unsigned int seed) {
    gen.seed(seed);
    // Random initialization
    std::uniform_real_distribution<double> dist(0.1, 1.0);
    for (int i = 0; i < num_states; ++i) {
        initial_probabilities[i] = dist(gen);
        for (int j = 0; j < num_states; ++j) transition_matrix.at(i, j) = dist(gen);
        for (int o = 0; o < num_observations; ++o) emission_matrix.at(i, o) = dist(gen);
    }
    // Normalize initial_probabilities
    double pi_sum = std::accumulate(initial_probabilities.begin(), initial_probabilities.end(), 0.0);
    for (int i = 0; i < num_states; ++i) initial_probabilities[i] /= pi_sum;
    // Normalize rows of transition_matrix and emission_matrix
    for (int i = 0; i < num_states; ++i) {
        double row_sum_A = 0.0, row_sum_B = 0.0;
        for (int j = 0; j < num_states; ++j) row_sum_A += transition_matrix.at(i, j);
        for (int o = 0; o < num_observations; ++o) row_sum_B += emission_matrix.at(i, o);
        for (int j = 0; j < num_states; ++j) transition_matrix.at(i, j) /= row_sum_A;
        for (int o = 0; o < num_observations; ++o) emission_matrix.at(i, o) /= row_sum_B;
    }

    double prev_ll = -std::numeric_limits<double>::infinity();
    for (int iter = 0; iter < max_iterations; ++iter) {
        std::vector<double> new_pi(num_states, smoothing_factor);
        mytrix::DenseMatrix new_A(num_states, num_states);
        mytrix::DenseMatrix new_B(num_states, num_observations);
        for (int i = 0; i < num_states; ++i) {
            for (int j = 0; j < num_states; ++j) new_A.at(i, j) = smoothing_factor;
            for (int o = 0; o < num_observations; ++o) new_B.at(i, o) = smoothing_factor;
        }
        double total_ll = 0.0;

        for (const auto& obs : observation_sequences) {
            if (obs.empty()) continue;
            int T = static_cast<int>(obs.size());
            auto alpha = forward_pass(obs);
            auto beta = backward_pass(obs);
            double ll = -std::numeric_limits<double>::infinity();
            for (int s = 0; s < num_states; ++s) ll = log_sum_exp(ll, alpha.at(s, T-1));
            total_ll += ll;

            // Gamma and Xi
            for (int t = 0; t < T; ++t) {
                for (int s = 0; s < num_states; ++s) {
                    double gamma_s = std::exp(alpha.at(s, t) + beta.at(s, t) - ll);
                    if (t == 0) new_pi[s] += gamma_s;
                    new_B.at(s, obs[t]) += gamma_s;
                    if (t < T - 1) {
                        for (int j = 0; j < num_states; ++j) {
                            double xi = std::exp(alpha.at(s, t) + std::log(transition_matrix.at(s, j)) +
                                        std::log(emission_matrix.at(j, obs[t+1])) + beta.at(j, t+1) - ll);
                            new_A.at(s, j) += xi;
                        }
                    }
                }
            }
        }

        // Normalize
        double pi_sum = std::accumulate(new_pi.begin(), new_pi.end(), 0.0);
        for (int i = 0; i < num_states; ++i) initial_probabilities[i] = new_pi[i] / pi_sum;
        for (int i = 0; i < num_states; ++i) {
            double row_sum_A = 0.0, row_sum_B = 0.0;
            for (int j = 0; j < num_states; ++j) row_sum_A += new_A.at(i, j);
            for (int o = 0; o < num_observations; ++o) row_sum_B += new_B.at(i, o);
            for (int j = 0; j < num_states; ++j) transition_matrix.at(i, j) = (row_sum_A > 0) ? new_A.at(i, j) / row_sum_A : 0.0;
            for (int o = 0; o < num_observations; ++o) emission_matrix.at(i, o) = (row_sum_B > 0) ? new_B.at(i, o) / row_sum_B : 0.0;
        }

        if (std::abs(total_ll - prev_ll) < tolerance) break;
        prev_ll = total_ll;
    }
}
