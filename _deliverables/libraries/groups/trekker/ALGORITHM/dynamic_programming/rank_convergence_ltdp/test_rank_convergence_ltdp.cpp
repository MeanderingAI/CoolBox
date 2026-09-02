#include <tyst_framework.hpp>

#include "rank_convergence_ltdp.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace {

using Solver = trekker::algorithm::dynamic_programming::RankConvergenceLTDP<double>;
using Matrix = Solver::Matrix;
using Vector = Solver::Vector;
using Stages = Solver::Stages;

Matrix rank_one_matrix(std::size_t states, double seed) {
    Vector col(states, 0.0);
    Vector row(states, 0.0);

    for (std::size_t i = 0; i < states; ++i) {
        col[i] = seed + static_cast<double>(i) * 0.1;
        row[i] = static_cast<double>(i) * 0.05;
    }

    Matrix out(states, Vector(states, 0.0));
    for (std::size_t i = 0; i < states; ++i) {
        for (std::size_t j = 0; j < states; ++j) {
            out[i][j] = col[i] + row[j];
        }
    }
    return out;
}

Stages make_rank_converging_stages(std::size_t states, std::size_t stage_count) {
    Stages stages;
    stages.reserve(stage_count);
    for (std::size_t i = 0; i < stage_count; ++i) {
        stages.push_back(rank_one_matrix(states, 0.2 * static_cast<double>(i + 1)));
    }
    return stages;
}

} // namespace

TEST(RankConvergenceLTDPTest, ParallelVectorsDetected) {
    Vector a{1.0, 2.0, 3.0};
    Vector b{4.0, 5.0, 6.0};
    Vector c{4.0, 6.0, 6.0};

    EXPECT_TRUE(Solver::are_parallel(a, b));
    EXPECT_FALSE(Solver::are_parallel(a, c));
}

TEST(RankConvergenceLTDPTest, MultiplyAndPredecessorWorkInTropicalSemiring) {
    Matrix m{
        {0.0, 1.0, 2.0},
        {2.0, 0.0, 1.0},
        {1.0, 2.0, 0.0},
    };
    Vector s{0.5, 1.5, -0.5};

    const Vector out = Solver::multiply(m, s);
    const auto pred = Solver::predecessor_product(m, s);

    EXPECT_NEAR(out[0], 2.5, 1e-9);
    EXPECT_NEAR(out[1], 2.5, 1e-9);
    EXPECT_NEAR(out[2], 3.5, 1e-9);

    EXPECT_EQ(pred[0], 1u);
    EXPECT_EQ(pred[1], 0u);
    EXPECT_EQ(pred[2], 1u);
}

TEST(RankConvergenceLTDPTest, RankConvergenceMatchesSequentialBaseline) {
    const std::size_t states = 5;
    const std::size_t stages_count = 24;

    const Stages stages = make_rank_converging_stages(states, stages_count);
    const Vector s0(states, 0.0);
    const Vector nz(states, 1.0);

    const auto seq = Solver::forward_sequential(stages, s0);
    const auto par = Solver::forward_rank_convergence(stages, s0, 4, nz);

    EXPECT_EQ(seq.stage_vectors.size(), par.stage_vectors.size());
    EXPECT_GT(par.fixup_iterations, 0u);

    for (std::size_t i = 1; i < seq.stage_vectors.size(); ++i) {
        EXPECT_TRUE(Solver::are_parallel(seq.stage_vectors[i], par.stage_vectors[i]));
        EXPECT_EQ(seq.predecessors[i], par.predecessors[i]);
    }
}

TEST(RankConvergenceLTDPTest, RejectsNzWithMinusInfinityEntries) {
    const std::size_t states = 3;
    const Stages stages = make_rank_converging_stages(states, 4);
    const Vector s0(states, 0.0);
    Vector nz(states, 0.0);
    nz[1] = Solver::neg_inf();

    EXPECT_THROW(Solver::forward_rank_convergence(stages, s0, 2, nz), std::invalid_argument);
}
