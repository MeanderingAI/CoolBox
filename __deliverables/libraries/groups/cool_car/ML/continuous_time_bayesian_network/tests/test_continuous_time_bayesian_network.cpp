#include "tyst_framework.hpp"

#include "continuous_time_bayesian_network.h"

#include <numeric>

using namespace ml;

TYST_TEST(ContinuousTimeBayesianNetworkTests, TransitionMatrixIsRowStochastic) {
    ContinuousTimeBayesianNetwork ctbn;
    ctbn.set_intensity_matrix({
        {-2.0, 2.0},
        {1.0, -1.0}
    });

    const auto transition = ctbn.transition_matrix(0.5);

    TYST_EXPECT_EQ(transition.size(), static_cast<std::size_t>(2));
    for (const auto& row : transition) {
        const double row_sum = std::accumulate(row.begin(), row.end(), 0.0);
        TYST_EXPECT_NEAR(row_sum, 1.0, 1e-7);
        for (double value : row) {
            TYST_EXPECT_GE(value, -1e-10);
        }
    }
}

TYST_TEST(ContinuousTimeBayesianNetworkTests, PropagationMovesProbabilityMassWithPositiveRates) {
    ContinuousTimeBayesianNetwork ctbn;
    ctbn.set_intensity_matrix({
        {-4.0, 4.0},
        {1.0, -1.0}
    });

    const std::vector<double> next = ctbn.propagate({1.0, 0.0}, 0.5);
    TYST_EXPECT_TRUE(next[1] > 0.0);
    TYST_EXPECT_TRUE(next[0] < 1.0);
    TYST_EXPECT_NEAR(next[0] + next[1], 1.0, 1e-8);
}

TYST_TEST(ContinuousTimeBayesianNetworkTests, ZeroStepReturnsUnchangedBelief) {
    ContinuousTimeBayesianNetwork ctbn;
    ctbn.set_intensity_matrix({
        {-2.0, 2.0},
        {1.0, -1.0}
    });

    const std::vector<double> prior = {0.25, 0.75};
    const std::vector<double> posterior = ctbn.propagate(prior, 0.0);

    TYST_EXPECT_NEAR(prior[0], posterior[0], 1e-10);
    TYST_EXPECT_NEAR(prior[1], posterior[1], 1e-10);
}
