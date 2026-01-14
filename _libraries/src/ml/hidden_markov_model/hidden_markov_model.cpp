#include <iostream>
#include <cmath>
#include <limits>

#include <random>

#include "ML/hidden_markov_model/hidden_markov_model.h"

// Static helper function to compute log(a + b) from log(a) and log(b)
double HMM::log_sum_exp(double log_a, double log_b) {
    if (log_a == -std::numeric_limits<double>::infinity()) {
        return log_b;
    }
    if (log_b == -std::numeric_limits<double>::infinity()) {
        return log_a;
    }
    double max_val = std::max(log_a, log_b);
    return max_val + std::log(std::exp(log_a - max_val) + std::exp(log_b - max_val));
}

// Constructor
HMM::HMM(int states, int observations) 
    : num_states(states), num_observations(observations), gen(std::random_device{}()) {
    // Initialize matrices with zeros or random values
    initial_probabilities = Eigen::VectorXd::Zero(num_states);
    transition_matrix = Eigen::MatrixXd::Zero(num_states, num_states);
    emission_matrix = Eigen::MatrixXd::Zero(num_states, num_observations);

}

// Setters
void HMM::set_initial_probabilities(const Eigen::VectorXd& pi) {
    initial_probabilities = pi;
}

void HMM::set_transition_matrix(const Eigen::MatrixXd& A) {
    transition_matrix = A;
}

void HMM::set_emission_matrix(const Eigen::MatrixXd& B) {
    emission_matrix = B;
}

// Getters
Eigen::VectorXd HMM::get_initial_probabilities() const {
    return initial_probabilities;
}

Eigen::MatrixXd HMM::get_transition_matrix() const {
    return transition_matrix;
}

Eigen::MatrixXd HMM::get_emission_matrix() const {
    return emission_matrix;
}

// Problem 1: Forward Algorithm (Evaluation)
double HMM::log_likelihood(const std::vector<int>& observations) const {
    int T = observations.size();
    Eigen::MatrixXd alpha = forward_pass(observations);
    
    // The final log likelihood is the sum of the last column of alpha
    double final_log_prob = -std::numeric_limits<double>::infinity();
    for (int i = 0; i < num_states; ++i) {
        final_log_prob = log_sum_exp(final_log_prob, alpha(i, T - 1));
    }
    return final_log_prob;
}

// Problem 2: Viterbi Algorithm (Decoding)
std::vector<int> HMM::get_most_likely_states(const std::vector<int>& observations) const {
    int T = observations.size();
    // Viterbi path probability matrix (in log-domain)
    Eigen::MatrixXd delta(num_states, T); 
    // Backpointer matrix to reconstruct the path
    Eigen::MatrixXi psi(num_states, T);

    // 1. Initialization
    for (int i = 0; i < num_states; ++i) {
        delta(i, 0) = std::log(initial_probabilities(i)) + std::log(emission_matrix(i, observations[0]));
        psi(i, 0) = 0; // Or some sentinel value
    }

    // 2. Recursion
    for (int t = 1; t < T; ++t) {
        for (int j = 0; j < num_states; ++j) {
            double max_log_prob = -std::numeric_limits<double>::infinity();
            int max_prev_state = -1;
            
            for (int i = 0; i < num_states; ++i) {
                double current_prob = delta(i, t - 1) + std::log(transition_matrix(i, j));
                if (current_prob > max_log_prob) {
                    max_log_prob = current_prob;
                    max_prev_state = i;
                }
            }
            delta(j, t) = max_log_prob + std::log(emission_matrix(j, observations[t]));
            psi(j, t) = max_prev_state;
        }
    }

    // 3. Termination: Find the end of the most likely path
    std::vector<int> state_sequence(T);
    double max_prob = -std::numeric_limits<double>::infinity();
    int last_state = -1;

    for (int i = 0; i < num_states; ++i) {
        if (delta(i, T - 1) > max_prob) {
            max_prob = delta(i, T - 1);
            last_state = i;
        }
    }
    state_sequence[T - 1] = last_state;

    // 4. Backtracking
    for (int t = T - 2; t >= 0; --t) {
        state_sequence[t] = psi(state_sequence[t + 1], t + 1);
    }

    return state_sequence;
}


// Problem 3: Baum-Welch Algorithm (Training)
void HMM::train(const std::vector<std::vector<int>>& observation_sequences, int max_iterations, double tolerance, double smoothing_factor, unsigned int seed) {
    if (seed != 0) {
        gen.seed(seed);
    }

    std::uniform_real_distribution<> dis(0.01, 1.0);
    
    // Initializing with random probabilities
    for (int i = 0; i < num_states; ++i) {
        initial_probabilities(i) = dis(gen);
    }
    initial_probabilities /= initial_probabilities.sum();

    for (int i = 0; i < num_states; ++i) {
        for (int j = 0; j < num_states; ++j) {
            transition_matrix(i, j) = dis(gen);
        }
    }
    transition_matrix.rowwise().normalize();

    for (int i = 0; i < num_states; ++i) {
        for (int j = 0; j < num_observations; ++j) {
            emission_matrix(i, j) = dis(gen);
        }
    }
    emission_matrix.rowwise().normalize();

    double prev_log_likelihood = -std::numeric_limits<double>::infinity();

    for (int iter = 0; iter < max_iterations; ++iter) {
        // E-step: Compute expected frequencies
        Eigen::VectorXd expected_pi_numerator = Eigen::VectorXd::Zero(num_states);
        Eigen::MatrixXd expected_A_numerator = Eigen::MatrixXd::Zero(num_states, num_states);
        Eigen::MatrixXd expected_B_numerator = Eigen::MatrixXd::Zero(num_states, num_observations);
        
        Eigen::VectorXd expected_state_counts_A = Eigen::VectorXd::Zero(num_states);
        Eigen::VectorXd expected_state_counts_B = Eigen::VectorXd::Zero(num_states);

        double total_log_likelihood = -std::numeric_limits<double>::infinity();

        for (const auto& observations : observation_sequences) {
            int T = observations.size();
            Eigen::MatrixXd alpha = forward_pass(observations);
            Eigen::MatrixXd beta = backward_pass(observations);
            
            // Calculate sequence log probability
            double sequence_log_prob = -std::numeric_limits<double>::infinity();
            for (int i = 0; i < num_states; ++i) {
                sequence_log_prob = log_sum_exp(sequence_log_prob, alpha(i, T - 1));
            }
            total_log_likelihood = log_sum_exp(total_log_likelihood, sequence_log_prob);

            // Gamma: P(q_t = i | O, lambda)
            Eigen::MatrixXd gamma(num_states, T);
            for (int t = 0; t < T; ++t) {
                for (int i = 0; i < num_states; ++i) {
                    gamma(i, t) = std::exp(alpha(i, t) + beta(i, t) - sequence_log_prob);
                }
            }

            // Accumulate expected counts
            expected_pi_numerator += gamma.col(0);
            
            for(int i = 0; i < num_states; ++i) {
                for(int j = 0; j < num_states; ++j) {
                    for(int t = 0; t < T - 1; ++t) {
                        double xi_ij_t_log = alpha(i, t) + std::log(transition_matrix(i, j)) + std::log(emission_matrix(j, observations[t + 1])) + beta(j, t + 1);
                        expected_A_numerator(i, j) += std::exp(xi_ij_t_log - sequence_log_prob);
                    }
                }
            }

            for(int i = 0; i < num_states; ++i) {
                for(int k = 0; k < num_observations; ++k) {
                    for(int t = 0; t < T; ++t) {
                        if (observations[t] == k) {
                            expected_B_numerator(i, k) += gamma(i, t);
                        }
                    }
                }
            }
            
            for(int i = 0; i < num_states; ++i) {
                for(int t = 0; t < T - 1; ++t) {
                    expected_state_counts_A(i) += gamma(i, t);
                }
            }
            
            for(int i = 0; i < num_states; ++i) {
                for(int t = 0; t < T; ++t) {
                    expected_state_counts_B(i) += gamma(i, t);
                }
            }
        }
        
        // M-step: Re-estimate model parameters with Laplace Smoothing
        initial_probabilities = (expected_pi_numerator.array() + smoothing_factor) / (expected_pi_numerator.sum() + smoothing_factor * num_states);
        
        for (int i = 0; i < num_states; ++i) {
            double denominator_A = expected_state_counts_A(i) + smoothing_factor * num_states;
            if (denominator_A > 0) {
                transition_matrix.row(i) = (expected_A_numerator.row(i).array() + smoothing_factor) / denominator_A;
            } else {
                // If denominator is zero even with smoothing, reset to uniform probabilities
                transition_matrix.row(i).setConstant(1.0 / num_states);
            }
            
            double denominator_B = expected_state_counts_B(i) + smoothing_factor * num_observations;
            if (denominator_B > 0) {
                emission_matrix.row(i) = (expected_B_numerator.row(i).array() + smoothing_factor) / denominator_B;
            } else {
                // If denominator is zero even with smoothing, reset to uniform probabilities
                emission_matrix.row(i).setConstant(1.0 / num_observations);
            }
        }
        
        // Check for convergence
        if (std::abs(total_log_likelihood - prev_log_likelihood) < tolerance) {
            std::cout << "Convergence reached after " << iter << " iterations." << std::endl;
            break;
        }
        prev_log_likelihood = total_log_likelihood;
    }
}

// Forward pass helper method (in log-domain for stability)
Eigen::MatrixXd HMM::forward_pass(const std::vector<int>& observations) const {
    int T = observations.size();
    if (T == 0) {
        return Eigen::MatrixXd::Zero(num_states, 0);
    }
    
    Eigen::MatrixXd alpha(num_states, T);
    
    // Initialization: P(O_1, q_1=i) = P(q_1=i) * P(O_1 | q_1=i)
    for (int i = 0; i < num_states; ++i) {
        double log_pi = std::log(initial_probabilities(i));
        double log_b = std::log(emission_matrix(i, observations[0]));
        alpha(i, 0) = log_pi + log_b;
    }
    
    // Recursion: P(O_1...O_t, q_t=j) = [sum_i(P(O_1...O_{t-1}, q_{t-1}=i) * P(q_t=j | q_{t-1}=i))] * P(O_t | q_t=j)
    for (int t = 1; t < T; ++t) {
        for (int j = 0; j < num_states; ++j) {
            double sum_log_probs = -std::numeric_limits<double>::infinity();
            for (int i = 0; i < num_states; ++i) {
                sum_log_probs = log_sum_exp(sum_log_probs, alpha(i, t - 1) + std::log(transition_matrix(i, j)));
            }
            alpha(j, t) = sum_log_probs + std::log(emission_matrix(j, observations[t]));
        }
    }
    return alpha;
}

// Backward pass helper method (in log-domain for stability)
Eigen::MatrixXd HMM::backward_pass(const std::vector<int>& observations) const {
    int T = observations.size();
    Eigen::MatrixXd beta(num_states, T);

    // Initialization
    for (int i = 0; i < num_states; ++i) {
        beta(i, T - 1) = 0.0; // log(1.0) = 0.0
    }

    // Recursion
    for (int t = T - 2; t >= 0; --t) {
        for (int i = 0; i < num_states; ++i) {
            double sum_log_probs = -std::numeric_limits<double>::infinity();
            for (int j = 0; j < num_states; ++j) {
                sum_log_probs = log_sum_exp(sum_log_probs, std::log(transition_matrix(i, j)) + std::log(emission_matrix(j, observations[t + 1])) + beta(j, t + 1));
            }
            beta(i, t) = sum_log_probs;
        }
    }
    return beta;
}