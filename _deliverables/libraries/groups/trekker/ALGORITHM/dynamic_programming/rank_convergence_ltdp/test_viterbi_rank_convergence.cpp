#include <tyst_framework.hpp>

#include "viterbi_rank_convergence.h"

#include <stdexcept>
#include <vector>

namespace {

using Decoder = trekker::algorithm::dynamic_programming::ViterbiRankConvergence<double>;
using Vector = Decoder::Vector;
using Matrix = Decoder::Matrix;

} // namespace

TEST(ViterbiRankConvergenceTest, MatchesSequentialOnDeterministicTwoStateExample) {
    const std::vector<std::size_t> observations{0, 1, 0, 1};

    const Vector initial_log{
        -0.01,
        -4.0,
    };

    const Matrix transition_log{
        {-0.05, -3.0},
        {-2.5, -0.05},
    };

    const Matrix emission_log{
        {-0.02, -3.5},
        {-3.0, -0.02},
    };

    const Vector nz{1.0, 1.0};

    const auto seq = Decoder::decode_sequential(observations, initial_log, transition_log, emission_log);
    const auto par = Decoder::decode_rank_convergence(observations, initial_log, transition_log, emission_log, 2, nz);

    EXPECT_EQ(seq.path.size(), observations.size());
    EXPECT_EQ(seq.path, par.path);
    EXPECT_NEAR(seq.log_score, par.log_score, 1e-9);
    EXPECT_GT(par.fixup_iterations, 0u);
}

TEST(ViterbiRankConvergenceTest, SupportsSingleObservationWithoutStages) {
    const std::vector<std::size_t> observations{1};

    const Vector initial_log{-0.4, -0.1, -2.0};
    const Matrix transition_log{
        {-0.1, -0.2, -1.0},
        {-0.4, -0.1, -0.9},
        {-0.5, -0.2, -0.2},
    };
    const Matrix emission_log{
        {-2.0, -0.2},
        {-2.0, -0.7},
        {-2.0, -1.5},
    };

    const auto result = Decoder::decode_sequential(observations, initial_log, transition_log, emission_log);
    EXPECT_EQ(result.path.size(), 1u);
    EXPECT_EQ(result.path[0], 0u);
    EXPECT_NEAR(result.log_score, -0.6, 1e-9);
}

TEST(ViterbiRankConvergenceTest, RejectsInvalidObservationIndex) {
    const std::vector<std::size_t> observations{0, 2};
    const Vector initial_log{-0.1, -0.2};
    const Matrix transition_log{
        {-0.1, -0.3},
        {-0.2, -0.1},
    };
    const Matrix emission_log{
        {-0.1, -2.0},
        {-2.0, -0.1},
    };
    const Vector nz{1.0, 1.0};

    EXPECT_THROW(
        Decoder::decode_rank_convergence(observations, initial_log, transition_log, emission_log, 2, nz),
        std::invalid_argument);
}
