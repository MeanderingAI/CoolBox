#include "dynamic_bayesian_network.h"

#include <cmath>
#include <numeric>
#include <stdexcept>

namespace {

constexpr double kEpsilon = 1e-12;

bool is_non_negative(double value) {
    return value >= -kEpsilon;
}

} // namespace

namespace ml {

DynamicBayesianNetwork::DynamicBayesianNetwork()
    : states_{"False", "True"} {
    reset_defaults();
}

void DynamicBayesianNetwork::set_states(const std::vector<std::string>& states) {
    if (states.empty()) {
        throw std::invalid_argument("State list must not be empty");
    }
    states_ = states;
    reset_defaults();
}

const std::vector<std::string>& DynamicBayesianNetwork::get_states() const {
    return states_;
}

void DynamicBayesianNetwork::set_initial_distribution(const std::vector<double>& initial_distribution) {
    if (initial_distribution.size() != states_.size()) {
        throw std::invalid_argument("Initial distribution size must match number of states");
    }
    initial_distribution_ = normalise_distribution(initial_distribution);
}

const std::vector<double>& DynamicBayesianNetwork::get_initial_distribution() const {
    return initial_distribution_;
}

void DynamicBayesianNetwork::set_transition_matrix(const std::vector<std::vector<double>>& transition_matrix) {
    validate_square_matrix(transition_matrix, states_.size());

    transition_matrix_.clear();
    transition_matrix_.reserve(transition_matrix.size());
    for (const auto& row : transition_matrix) {
        transition_matrix_.push_back(normalise_distribution(row));
    }
}

const std::vector<std::vector<double>>& DynamicBayesianNetwork::get_transition_matrix() const {
    return transition_matrix_;
}

std::vector<double> DynamicBayesianNetwork::predict_next(const std::vector<double>& belief) const {
    if (belief.size() != states_.size()) {
        throw std::invalid_argument("Belief size must match number of states");
    }

    const std::vector<double> normalised_belief = normalise_distribution(belief);
    std::vector<double> predicted(states_.size(), 0.0);

    for (std::size_t from_state = 0; from_state < states_.size(); ++from_state) {
        for (std::size_t to_state = 0; to_state < states_.size(); ++to_state) {
            predicted[to_state] += normalised_belief[from_state] * transition_matrix_[from_state][to_state];
        }
    }

    return normalise_distribution(predicted);
}

std::vector<double> DynamicBayesianNetwork::update_with_evidence(
    const std::vector<double>& predicted_belief,
    const std::vector<double>& evidence_likelihood
) const {
    if (predicted_belief.size() != states_.size() || evidence_likelihood.size() != states_.size()) {
        throw std::invalid_argument("Belief and evidence vectors must match number of states");
    }

    std::vector<double> posterior(states_.size(), 0.0);
    for (std::size_t state = 0; state < states_.size(); ++state) {
        if (!is_non_negative(evidence_likelihood[state])) {
            throw std::invalid_argument("Evidence likelihood must be non-negative");
        }
        posterior[state] = predicted_belief[state] * evidence_likelihood[state];
    }

    return normalise_distribution(posterior);
}

std::vector<std::vector<double>> DynamicBayesianNetwork::forward_filter(
    const std::vector<std::vector<double>>& evidence_likelihoods
) const {
    std::vector<std::vector<double>> beliefs;
    beliefs.reserve(evidence_likelihoods.size());

    std::vector<double> current = initial_distribution_;
    for (const auto& likelihood : evidence_likelihoods) {
        const std::vector<double> predicted = predict_next(current);
        current = update_with_evidence(predicted, likelihood);
        beliefs.push_back(current);
    }

    return beliefs;
}

void DynamicBayesianNetwork::reset_defaults() {
    const std::size_t state_count = states_.size();
    initial_distribution_.assign(state_count, 1.0 / static_cast<double>(state_count));

    transition_matrix_.assign(state_count, std::vector<double>(state_count, 0.0));
    for (std::size_t row = 0; row < state_count; ++row) {
        for (std::size_t col = 0; col < state_count; ++col) {
            transition_matrix_[row][col] = 1.0 / static_cast<double>(state_count);
        }
    }
}

std::vector<double> DynamicBayesianNetwork::normalise_distribution(const std::vector<double>& values) {
    if (values.empty()) {
        throw std::invalid_argument("Distribution must not be empty");
    }

    for (double value : values) {
        if (!is_non_negative(value)) {
            throw std::invalid_argument("Distribution contains a negative value");
        }
    }

    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    if (sum <= kEpsilon) {
        throw std::invalid_argument("Distribution sum must be positive");
    }

    std::vector<double> normalised(values.size(), 0.0);
    for (std::size_t i = 0; i < values.size(); ++i) {
        normalised[i] = values[i] / sum;
    }
    return normalised;
}

void DynamicBayesianNetwork::validate_square_matrix(
    const std::vector<std::vector<double>>& matrix,
    std::size_t size
) {
    if (matrix.size() != size) {
        throw std::invalid_argument("Transition matrix must be square with one row per state");
    }

    for (const auto& row : matrix) {
        if (row.size() != size) {
            throw std::invalid_argument("Transition matrix must be square with one column per state");
        }
    }
}

} // namespace ml
