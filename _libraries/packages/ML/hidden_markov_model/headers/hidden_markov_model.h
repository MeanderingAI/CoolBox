#ifndef HIDDEN_MARKOV_MODEL_H
#define HIDDEN_MARKOV_MODEL_H

#include <vector>
#include <numeric>
#include <random>
#include <Eigen/Dense>

class HMM {
private:
    int num_states;
    int num_observations;
    
    Eigen::VectorXd initial_probabilities; // pi vector
    Eigen::MatrixXd transition_matrix;     // A matrix
    Eigen::MatrixXd emission_matrix;       // B matrix

    std::mt19937 gen;

    static double log_sum_exp(double log_a, double log_b);
    Eigen::MatrixXd forward_pass(const std::vector<int>& observations) const;
    Eigen::MatrixXd backward_pass(const std::vector<int>& observations) const;
public:
    // Constructor to initialize the HMM with its parameters
    HMM(int states, int observations);
    
    // Setters for the model parameters using Eigen types
    void set_initial_probabilities(const Eigen::VectorXd& pi);
    void set_transition_matrix(const Eigen::MatrixXd& A);
    void set_emission_matrix(const Eigen::MatrixXd& B);
    
    // Getters for the model parameters
    Eigen::VectorXd get_initial_probabilities() const;
    Eigen::MatrixXd get_transition_matrix() const;
    Eigen::MatrixXd get_emission_matrix() const;
    
    // Core HMM Algorithms
    
    // Problem 1: Evaluation (Forward Algorithm)
    // Returns the log probability to avoid underflow issues with very small numbers
    double log_likelihood(const std::vector<int>& observations) const;
    
    // Problem 2: Decoding (Viterbi Algorithm)
    // Finds the most likely hidden state sequence for a given observation sequence
    // get_most_likely_states
    std::vector<int> get_most_likely_states(const std::vector<int>& observations) const;
    
    // Problem 3: Training (Baum-Welch Algorithm)
    // Re-estimates the HMM parameters from a set of observation sequences
    void train(const std::vector<std::vector<int>>& observation_sequences, int max_iterations = 100, double tolerance = 1e-6, double smoothing_factor = 0, unsigned int seed = 0);
};

#endif // HIDDEN_MARKOV_MODEL_H
