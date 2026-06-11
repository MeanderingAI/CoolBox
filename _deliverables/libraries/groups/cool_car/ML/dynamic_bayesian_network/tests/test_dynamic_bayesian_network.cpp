#include "tyst_framework.hpp"

#include "dynamic_bayesian_network.h"

#include <cmath>

using namespace ml;

TYST_TEST(DynamicBayesianNetworkTests, PredictAndUpdateProduceNormalisedPosterior) {
    DynamicBayesianNetwork dbn;
    dbn.set_initial_distribution({0.9, 0.1});
    dbn.set_transition_matrix({
        {0.8, 0.2},
        {0.3, 0.7}
    });

    const std::vector<double> predicted = dbn.predict_next(dbn.get_initial_distribution());
    TYST_EXPECT_NEAR(predicted[0], 0.75, 1e-9);
    TYST_EXPECT_NEAR(predicted[1], 0.25, 1e-9);

    const std::vector<double> posterior = dbn.update_with_evidence(predicted, {0.1, 0.9});
    TYST_EXPECT_NEAR(posterior[0], 0.25, 1e-9);
    TYST_EXPECT_NEAR(posterior[1], 0.75, 1e-9);
}

TYST_TEST(DynamicBayesianNetworkTests, ForwardFilterTracksBeliefOverMultipleSteps) {
    DynamicBayesianNetwork dbn;
    dbn.set_initial_distribution({0.5, 0.5});
    dbn.set_transition_matrix({
        {0.9, 0.1},
        {0.2, 0.8}
    });

    const std::vector<std::vector<double>> filtered = dbn.forward_filter({
        {0.7, 0.3},
        {0.4, 0.6}
    });

    TYST_EXPECT_EQ(filtered.size(), static_cast<std::size_t>(2));
    for (const auto& belief : filtered) {
        const double sum = belief[0] + belief[1];
        TYST_EXPECT_NEAR(sum, 1.0, 1e-9);
    }

    TYST_EXPECT_TRUE(filtered[0][0] > filtered[1][0]);
    TYST_EXPECT_TRUE(filtered[1][1] > filtered[0][1]);
}
