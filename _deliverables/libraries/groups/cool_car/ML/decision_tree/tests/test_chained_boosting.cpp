#include "tyst_framework.hpp"

#include "chained_boosting.h"

#include <vector>

namespace {

ChainedBoosting::TrainingFeatures make_pipeline_features() {
    ChainedBoosting::TrainingFeatures features;
    for (int repeat_index = 0; repeat_index < 8; ++repeat_index) {
        features.push_back({{0.0}, {0.0}});
        features.push_back({{1.0}, {1.0}});
    }
    return features;
}

ChainedBoosting::CostMatrix make_pipeline_costs() {
    ChainedBoosting::CostMatrix costs;
    for (int repeat_index = 0; repeat_index < 8; ++repeat_index) {
        costs.push_back({0.0, 1.0, 3.0});
        costs.push_back({3.0, 2.0, 0.0});
    }
    return costs;
}

double always_pass_cost(const ChainedBoosting::CostMatrix& costs) {
    double total = 0.0;
    for (const auto& sample_costs : costs) {
        total += sample_costs.back();
    }
    return total / static_cast<double>(costs.size());
}

} // namespace

TYST_TEST(ChainedBoostingTest, LearnsSequentialStoppingDecisions) {
    ChainedBoostingParameters parameters;
    parameters.rounds = 12;
    ChainedBoosting model(parameters);
    const auto features = make_pipeline_features();
    const auto costs = make_pipeline_costs();

    model.fit(features, costs);

    TYST_EXPECT_EQ(model.stage_count(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(model.predict_stop_stage({{0.0}, {0.0}}), static_cast<std::size_t>(0));
    TYST_EXPECT_EQ(model.predict_stop_stage({{1.0}, {1.0}}), static_cast<std::size_t>(2));
    TYST_EXPECT_LT(model.empirical_cost(features, costs), always_pass_cost(costs));
}

TYST_TEST(ChainedBoostingTest, ExposesStageScoresAndRules) {
    ChainedBoosting model;
    const auto features = make_pipeline_features();
    const auto costs = make_pipeline_costs();

    model.fit(features, costs);
    const auto reject_scores = model.decision_scores({{0.0}, {0.0}});
    const auto pass_scores = model.decision_scores({{1.0}, {1.0}});

    TYST_EXPECT_EQ(reject_scores.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_TRUE(model.rules_per_stage(0) > 0);
    TYST_EXPECT_TRUE(reject_scores[0] > 0.0);
    TYST_EXPECT_TRUE(pass_scores[0] <= 0.0);
}