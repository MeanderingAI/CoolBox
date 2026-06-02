#ifndef HIDDEN_MARKOV_MODEL_H
#define HIDDEN_MARKOV_MODEL_H

#include <vector>
#include <numeric>
#include <random>
#include "mytrix_eigen_compat.hpp"

class HMM {
private:
    int num_states;
    int num_observations;

    std::vector<double> initial_probabilities;
    mytrix::DenseMatrix transition_matrix;
    mytrix::DenseMatrix emission_matrix;

    std::mt19937 gen;

    static double log_sum_exp(double log_a, double log_b);
    static double log_sum_exp(const std::vector<double>& vals);
    mytrix::DenseMatrix forward_pass(const std::vector<int>& observations) const;
    mytrix::DenseMatrix backward_pass(const std::vector<int>& observations) const;

public:
    HMM(int states, int observations);

    void set_initial_probabilities(const std::vector<double>& pi);
    void set_transition_matrix(const Eigen::MatrixXd& A);
    void set_emission_matrix(const Eigen::MatrixXd& B);

    std::vector<double> get_initial_probabilities() const;
    Eigen::MatrixXd get_transition_matrix() const;
    Eigen::MatrixXd get_emission_matrix() const;

    double log_likelihood(const std::vector<int>& observations) const;
    std::vector<int> get_most_likely_states(const std::vector<int>& observations) const;

    void train(const std::vector<std::vector<int>>& observation_sequences,
               int max_iterations = 100,
               double tolerance = 1e-6,
               double smoothing_factor = 0,
               unsigned int seed = 0);
};

#endif
