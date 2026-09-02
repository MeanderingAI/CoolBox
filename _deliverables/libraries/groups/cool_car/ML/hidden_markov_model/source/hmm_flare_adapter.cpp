#include "hmm_flare_adapter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

std::vector<double> softmax_slice(const ml::FlareState& values, std::size_t offset, std::size_t count) {
    if (offset + count > values.size()) {
        throw std::invalid_argument("HMM FLARE state is too small for softmax slice");
    }

    double max_value = values[offset];
    for (std::size_t i = 1; i < count; ++i) {
        max_value = std::max(max_value, values[offset + i]);
    }

    std::vector<double> probabilities(count, 0.0);
    double sum = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        probabilities[i] = std::exp(values[offset + i] - max_value);
        sum += probabilities[i];
    }
    for (double& probability : probabilities) {
        probability /= sum;
    }
    return probabilities;
}

std::vector<std::vector<int>> stride_sequences(const std::vector<std::vector<int>>& sequences,
                                               std::size_t stride) {
    if (stride == 0) {
        throw std::invalid_argument("HMM FLARE observation stride must be positive");
    }

    std::vector<std::vector<int>> reduced;
    reduced.reserve(sequences.size());
    for (const auto& sequence : sequences) {
        std::vector<int> subsampled;
        for (std::size_t i = 0; i < sequence.size(); i += stride) {
            subsampled.push_back(sequence[i]);
        }
        if (!subsampled.empty()) {
            reduced.push_back(std::move(subsampled));
        }
    }
    return reduced;
}

double dirichlet_log_prior(const std::vector<double>& probabilities, double concentration) {
    if (concentration <= 0.0) {
        throw std::invalid_argument("HMM FLARE Dirichlet concentration must be positive");
    }

    double result = 0.0;
    const double exponent = concentration - 1.0;
    for (double probability : probabilities) {
        result += exponent * std::log(std::max(probability, std::numeric_limits<double>::min()));
    }
    return result;
}

void validate_problem(const HmmFlareProblem& problem) {
    if (problem.states <= 0 || problem.observations <= 0) {
        throw std::invalid_argument("HMM FLARE problem requires positive state and observation counts");
    }
    if (problem.observation_sequences.empty()) {
        throw std::invalid_argument("HMM FLARE problem requires at least one observation sequence");
    }
    for (const auto& sequence : problem.observation_sequences) {
        for (int observation : sequence) {
            if (observation < 0 || observation >= problem.observations) {
                throw std::invalid_argument("HMM FLARE observation is outside the model observation range");
            }
        }
    }
}

} // namespace

std::size_t hmm_flare_parameter_count(int states, int observations) {
    if (states <= 0 || observations <= 0) {
        throw std::invalid_argument("HMM FLARE parameter count requires positive dimensions");
    }
    return static_cast<std::size_t>(states) +
           static_cast<std::size_t>(states * states) +
           static_cast<std::size_t>(states * observations);
}

HMM hmm_from_flare_state(const ml::FlareState& parameters, int states, int observations) {
    const std::size_t expected = hmm_flare_parameter_count(states, observations);
    if (parameters.size() != expected) {
        throw std::invalid_argument("HMM FLARE state size does not match HMM dimensions");
    }

    HMM model(states, observations);
    std::size_t offset = 0;

    model.set_initial_probabilities(softmax_slice(parameters, offset, static_cast<std::size_t>(states)));
    offset += static_cast<std::size_t>(states);

    mytrix::DenseMatrix transition(states, states);
    for (int row = 0; row < states; ++row) {
        const auto probabilities = softmax_slice(parameters, offset, static_cast<std::size_t>(states));
        offset += static_cast<std::size_t>(states);
        for (int col = 0; col < states; ++col) {
            transition.at(row, col) = probabilities[static_cast<std::size_t>(col)];
        }
    }
    model.set_transition_matrix(transition);

    mytrix::DenseMatrix emission(states, observations);
    for (int row = 0; row < states; ++row) {
        const auto probabilities = softmax_slice(parameters, offset, static_cast<std::size_t>(observations));
        offset += static_cast<std::size_t>(observations);
        for (int col = 0; col < observations; ++col) {
            emission.at(row, col) = probabilities[static_cast<std::size_t>(col)];
        }
    }
    model.set_emission_matrix(emission);

    return model;
}

double hmm_flare_log_prior(const ml::FlareState& parameters,
                           int states,
                           int observations,
                           const HmmFlarePrior& prior) {
    const std::size_t expected = hmm_flare_parameter_count(states, observations);
    if (parameters.size() != expected) {
        throw std::invalid_argument("HMM FLARE state size does not match HMM dimensions");
    }

    std::size_t offset = 0;
    double result = dirichlet_log_prior(softmax_slice(parameters, offset, static_cast<std::size_t>(states)),
                                        prior.initial_concentration);
    offset += static_cast<std::size_t>(states);

    for (int row = 0; row < states; ++row) {
        result += dirichlet_log_prior(softmax_slice(parameters, offset, static_cast<std::size_t>(states)),
                                      prior.transition_concentration);
        offset += static_cast<std::size_t>(states);
    }
    for (int row = 0; row < states; ++row) {
        result += dirichlet_log_prior(softmax_slice(parameters, offset, static_cast<std::size_t>(observations)),
                                      prior.emission_concentration);
        offset += static_cast<std::size_t>(observations);
    }
    return result;
}

ml::FlareLayer make_hmm_flare_layer(const std::string& name,
                                    const HmmFlareProblem& problem,
                                    std::size_t observation_stride,
                                    double tuning_omega) {
    validate_problem(problem);
    const auto sequences = stride_sequences(problem.observation_sequences, observation_stride);
    if (sequences.empty()) {
        throw std::invalid_argument("HMM FLARE fidelity produced no observations");
    }

    ml::FlareLayer layer;
    layer.name = name;
    layer.tuning_omega = tuning_omega;
    layer.evaluation_cost = 1.0 / static_cast<double>(observation_stride);
    layer.log_density = [problem, sequences](const ml::FlareState& parameters) {
        HMM model = hmm_from_flare_state(parameters, problem.states, problem.observations);
        double log_density = hmm_flare_log_prior(parameters, problem.states, problem.observations, problem.prior);
        for (const auto& sequence : sequences) {
            log_density += model.log_likelihood(sequence);
        }
        return log_density;
    };
    return layer;
}

std::vector<ml::FlareLayer> make_hmm_flare_layers(const HmmFlareProblem& problem,
                                                  const std::vector<std::size_t>& observation_strides) {
    if (observation_strides.empty()) {
        throw std::invalid_argument("HMM FLARE requires at least one fidelity stride");
    }

    std::vector<ml::FlareLayer> layers;
    layers.reserve(observation_strides.size());
    for (std::size_t i = 0; i < observation_strides.size(); ++i) {
        layers.push_back(make_hmm_flare_layer(
            "hmm_stride_" + std::to_string(observation_strides[i]),
            problem,
            observation_strides[i],
            i == 0 ? 0.0 : 1e-6
        ));
    }
    return layers;
}