#include "continuous_time_bayesian_network.h"

#include <cmath>
#include <numeric>
#include <stdexcept>

namespace {

constexpr double kEpsilon = 1e-12;

bool is_non_negative(double value) {
    return value >= -kEpsilon;
}

std::vector<std::vector<double>> identity_matrix(std::size_t size) {
    std::vector<std::vector<double>> identity(size, std::vector<double>(size, 0.0));
    for (std::size_t i = 0; i < size; ++i) {
        identity[i][i] = 1.0;
    }
    return identity;
}

std::vector<std::vector<double>> multiply_matrix(
    const std::vector<std::vector<double>>& left,
    const std::vector<std::vector<double>>& right
) {
    const std::size_t rows = left.size();
    const std::size_t inner = right.size();
    const std::size_t cols = right.front().size();

    std::vector<std::vector<double>> product(rows, std::vector<double>(cols, 0.0));
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t k = 0; k < inner; ++k) {
            const double left_value = left[i][k];
            for (std::size_t j = 0; j < cols; ++j) {
                product[i][j] += left_value * right[k][j];
            }
        }
    }
    return product;
}

void add_scaled_matrix(
    std::vector<std::vector<double>>& target,
    const std::vector<std::vector<double>>& source,
    double scale
) {
    for (std::size_t i = 0; i < target.size(); ++i) {
        for (std::size_t j = 0; j < target[i].size(); ++j) {
            target[i][j] += scale * source[i][j];
        }
    }
}

void normalise_rows(std::vector<std::vector<double>>& matrix) {
    for (auto& row : matrix) {
        for (double& value : row) {
            if (value < 0.0 && std::abs(value) < 1e-10) {
                value = 0.0;
            }
        }
        const double row_sum = std::accumulate(row.begin(), row.end(), 0.0);
        if (row_sum > kEpsilon) {
            for (double& value : row) {
                value /= row_sum;
            }
        }
    }
}

} // namespace

namespace ml {

ContinuousTimeBayesianNetwork::ContinuousTimeBayesianNetwork()
    : states_{"S0", "S1"} {
    reset_defaults();
}

void ContinuousTimeBayesianNetwork::set_states(const std::vector<std::string>& states) {
    if (states.empty()) {
        throw std::invalid_argument("State list must not be empty");
    }

    states_ = states;
    reset_defaults();
}

const std::vector<std::string>& ContinuousTimeBayesianNetwork::get_states() const {
    return states_;
}

void ContinuousTimeBayesianNetwork::set_intensity_matrix(const std::vector<std::vector<double>>& intensity_matrix) {
    validate_square_matrix(intensity_matrix, states_.size());

    for (std::size_t i = 0; i < intensity_matrix.size(); ++i) {
        double off_diagonal_sum = 0.0;
        for (std::size_t j = 0; j < intensity_matrix[i].size(); ++j) {
            const double value = intensity_matrix[i][j];
            if (i != j) {
                if (!is_non_negative(value)) {
                    throw std::invalid_argument("Off-diagonal intensity values must be non-negative");
                }
                off_diagonal_sum += value;
            }
        }

        const double diagonal = intensity_matrix[i][i];
        if (std::abs(diagonal + off_diagonal_sum) > 1e-9) {
            throw std::invalid_argument("Each diagonal intensity must equal negative sum of outgoing rates");
        }
    }

    intensity_matrix_ = intensity_matrix;
}

const std::vector<std::vector<double>>& ContinuousTimeBayesianNetwork::get_intensity_matrix() const {
    return intensity_matrix_;
}

std::vector<std::vector<double>> ContinuousTimeBayesianNetwork::transition_matrix(double dt, std::size_t series_terms) const {
    if (dt < 0.0) {
        throw std::invalid_argument("Time step must be non-negative");
    }
    if (series_terms == 0) {
        throw std::invalid_argument("Series terms must be greater than zero");
    }

    const std::size_t state_count = states_.size();
    std::vector<std::vector<double>> scaled_q = intensity_matrix_;
    for (std::size_t i = 0; i < state_count; ++i) {
        for (std::size_t j = 0; j < state_count; ++j) {
            scaled_q[i][j] *= dt;
        }
    }

    std::vector<std::vector<double>> result = identity_matrix(state_count);
    std::vector<std::vector<double>> term = identity_matrix(state_count);

    double factorial = 1.0;
    for (std::size_t k = 1; k <= series_terms; ++k) {
        term = multiply_matrix(term, scaled_q);
        factorial *= static_cast<double>(k);
        add_scaled_matrix(result, term, 1.0 / factorial);
    }

    normalise_rows(result);
    return result;
}

std::vector<double> ContinuousTimeBayesianNetwork::propagate(
    const std::vector<double>& belief,
    double dt,
    std::size_t series_terms
) const {
    if (belief.size() != states_.size()) {
        throw std::invalid_argument("Belief size must match number of states");
    }

    const std::vector<double> normalised_belief = normalise_distribution(belief);
    const std::vector<std::vector<double>> transition = transition_matrix(dt, series_terms);

    std::vector<double> next_belief(states_.size(), 0.0);
    for (std::size_t from_state = 0; from_state < states_.size(); ++from_state) {
        for (std::size_t to_state = 0; to_state < states_.size(); ++to_state) {
            next_belief[to_state] += normalised_belief[from_state] * transition[from_state][to_state];
        }
    }

    return normalise_distribution(next_belief);
}

std::vector<std::vector<double>> ContinuousTimeBayesianNetwork::propagate_trajectory(
    const std::vector<double>& initial_belief,
    const std::vector<double>& step_durations,
    std::size_t series_terms
) const {
    std::vector<std::vector<double>> beliefs;
    beliefs.reserve(step_durations.size() + 1);

    std::vector<double> current = normalise_distribution(initial_belief);
    beliefs.push_back(current);

    for (double dt : step_durations) {
        current = propagate(current, dt, series_terms);
        beliefs.push_back(current);
    }

    return beliefs;
}

void ContinuousTimeBayesianNetwork::reset_defaults() {
    const std::size_t state_count = states_.size();
    intensity_matrix_.assign(state_count, std::vector<double>(state_count, 0.0));

    if (state_count <= 1) {
        return;
    }

    const double off_diagonal_rate = 1.0 / static_cast<double>(state_count - 1);
    for (std::size_t i = 0; i < state_count; ++i) {
        double row_sum = 0.0;
        for (std::size_t j = 0; j < state_count; ++j) {
            if (i == j) {
                continue;
            }
            intensity_matrix_[i][j] = off_diagonal_rate;
            row_sum += off_diagonal_rate;
        }
        intensity_matrix_[i][i] = -row_sum;
    }
}

std::vector<double> ContinuousTimeBayesianNetwork::normalise_distribution(const std::vector<double>& values) {
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

void ContinuousTimeBayesianNetwork::validate_square_matrix(
    const std::vector<std::vector<double>>& matrix,
    std::size_t size
) {
    if (matrix.size() != size) {
        throw std::invalid_argument("Intensity matrix must be square with one row per state");
    }

    for (const auto& row : matrix) {
        if (row.size() != size) {
            throw std::invalid_argument("Intensity matrix must be square with one column per state");
        }
    }
}

} // namespace ml
