#pragma once

#include "rank_convergence_ltdp.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace trekker {
namespace algorithm {
namespace dynamic_programming {

template <typename T>
class ViterbiRankConvergence {
public:
    using value_type = T;
    using Vector = std::vector<T>;
    using Matrix = std::vector<Vector>;

    struct Result {
        T log_score = -std::numeric_limits<T>::infinity();
        std::vector<std::size_t> path;
        std::size_t fixup_iterations = 0;
    };

    static Result decode_sequential(
        const std::vector<std::size_t>& observations,
        const Vector& initial_log_probabilities,
        const Matrix& transition_log_probabilities,
        const Matrix& emission_log_probabilities) {
        return decode_impl(
            observations,
            initial_log_probabilities,
            transition_log_probabilities,
            emission_log_probabilities,
            false,
            1,
            Vector{});
    }

    static Result decode_rank_convergence(
        const std::vector<std::size_t>& observations,
        const Vector& initial_log_probabilities,
        const Matrix& transition_log_probabilities,
        const Matrix& emission_log_probabilities,
        std::size_t processors,
        const Vector& nz,
        std::size_t max_fixup_iterations = 0) {
        return decode_impl(
            observations,
            initial_log_probabilities,
            transition_log_probabilities,
            emission_log_probabilities,
            true,
            processors,
            nz,
            max_fixup_iterations);
    }

private:
    using LTDP = RankConvergenceLTDP<T>;
    using Stages = typename LTDP::Stages;

    static Result decode_impl(
        const std::vector<std::size_t>& observations,
        const Vector& initial_log_probabilities,
        const Matrix& transition_log_probabilities,
        const Matrix& emission_log_probabilities,
        bool use_rank_convergence,
        std::size_t processors,
        const Vector& nz,
        std::size_t max_fixup_iterations = 0) {
        validate_inputs(
            observations,
            initial_log_probabilities,
            transition_log_probabilities,
            emission_log_probabilities);

        const std::size_t states = initial_log_probabilities.size();
        Result result;
        result.path.assign(observations.size(), 0);

        Vector s0(states, LTDP::neg_inf());
        const std::size_t first_obs = observations.front();
        for (std::size_t state = 0; state < states; ++state) {
            s0[state] = plus_tropical(
                initial_log_probabilities[state],
                emission_log_probabilities[state][first_obs]);
        }

        if (observations.size() == 1) {
            const auto best = std::max_element(s0.begin(), s0.end());
            result.log_score = *best;
            result.path[0] = static_cast<std::size_t>(std::distance(s0.begin(), best));
            return result;
        }

        const Stages stages = build_viterbi_stages(observations, transition_log_probabilities, emission_log_probabilities);

        typename LTDP::ForwardResult forward;
        if (use_rank_convergence) {
            forward = LTDP::forward_rank_convergence(stages, s0, processors, nz, max_fixup_iterations);
        } else {
            forward = LTDP::forward_sequential(stages, s0);
        }

        const Vector& final_stage = forward.stage_vectors.back();
        const auto best = std::max_element(final_stage.begin(), final_stage.end());
        result.log_score = *best;
        result.path.back() = static_cast<std::size_t>(std::distance(final_stage.begin(), best));
        result.fixup_iterations = forward.fixup_iterations;

        for (std::size_t stage = stages.size(); stage >= 1; --stage) {
            result.path[stage - 1] = forward.predecessors[stage][result.path[stage]];
        }

        return result;
    }

    static Stages build_viterbi_stages(
        const std::vector<std::size_t>& observations,
        const Matrix& transition_log_probabilities,
        const Matrix& emission_log_probabilities) {
        const std::size_t states = transition_log_probabilities.size();
        Stages stages;
        stages.reserve(observations.size() - 1);

        for (std::size_t t = 1; t < observations.size(); ++t) {
            const std::size_t obs = observations[t];
            typename LTDP::Matrix stage(states, typename LTDP::Vector(states, LTDP::neg_inf()));

            // Matrix layout is [to_state][from_state].
            for (std::size_t to_state = 0; to_state < states; ++to_state) {
                for (std::size_t from_state = 0; from_state < states; ++from_state) {
                    const T trans = transition_log_probabilities[from_state][to_state];
                    const T emit = emission_log_probabilities[to_state][obs];
                    stage[to_state][from_state] = plus_tropical(trans, emit);
                }
            }

            stages.push_back(std::move(stage));
        }

        return stages;
    }

    static T plus_tropical(T a, T b) {
        if (a == LTDP::neg_inf() || b == LTDP::neg_inf()) {
            return LTDP::neg_inf();
        }
        return a + b;
    }

    static void validate_inputs(
        const std::vector<std::size_t>& observations,
        const Vector& initial_log_probabilities,
        const Matrix& transition_log_probabilities,
        const Matrix& emission_log_probabilities) {
        if (observations.empty()) {
            throw std::invalid_argument("observations must be non-empty");
        }
        if (initial_log_probabilities.empty()) {
            throw std::invalid_argument("initial_log_probabilities must be non-empty");
        }

        const std::size_t states = initial_log_probabilities.size();
        if (transition_log_probabilities.size() != states) {
            throw std::invalid_argument("transition matrix row count must match state count");
        }
        if (emission_log_probabilities.size() != states) {
            throw std::invalid_argument("emission matrix row count must match state count");
        }

        for (const auto& row : transition_log_probabilities) {
            if (row.size() != states) {
                throw std::invalid_argument("transition matrix must be square with state-count columns");
            }
        }

        std::size_t observation_alphabet = 0;
        for (const auto& row : emission_log_probabilities) {
            observation_alphabet = std::max(observation_alphabet, row.size());
        }
        if (observation_alphabet == 0) {
            throw std::invalid_argument("emission matrix must have at least one observation column");
        }
        for (const auto& row : emission_log_probabilities) {
            if (row.size() != observation_alphabet) {
                throw std::invalid_argument("all emission matrix rows must have the same number of columns");
            }
        }

        for (const std::size_t observation : observations) {
            if (observation >= observation_alphabet) {
                throw std::invalid_argument("observation index is out of range for emission matrix");
            }
        }
    }
};

} // namespace dynamic_programming
} // namespace algorithm
} // namespace trekker
