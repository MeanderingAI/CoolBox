#include "tyst_framework.hpp"

#include "catenary_support_vector_machine.h"

#include <vector>

namespace {

CatenarySupportVectorMachine::TrainingFeatures make_pipeline_features() {
    CatenarySupportVectorMachine::TrainingFeatures features;
    for (int repeat_index = 0; repeat_index < 10; ++repeat_index) {
        features.push_back({{-1.0}, {-1.0}});
        features.push_back({{1.0}, {1.0}});
    }
    return features;
}

CatenarySupportVectorMachine::CostMatrix make_pipeline_costs() {
    CatenarySupportVectorMachine::CostMatrix costs;
    for (int repeat_index = 0; repeat_index < 10; ++repeat_index) {
        costs.push_back({0.0, 1.0, 4.0});
        costs.push_back({4.0, 2.0, 0.0});
    }
    return costs;
}

double always_pass_cost(const CatenarySupportVectorMachine::CostMatrix& costs) {
    double total = 0.0;
    for (const auto& sample_costs : costs) total += sample_costs.back();
    return total / static_cast<double>(costs.size());
}

} // namespace

TYST_TEST(CatenarySupportVectorMachineTest, LearnsSequentialStoppingPolicy) {
    CatenarySVMParameters parameters;
    parameters.epochs = 80;
    parameters.learning_rate = 0.05;
    CatenarySupportVectorMachine model(parameters);
    const auto features = make_pipeline_features();
    const auto costs = make_pipeline_costs();

    model.fit(features, costs);

    TYST_EXPECT_EQ(model.stage_count(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(model.predict_stop_stage({{-1.0}, {-1.0}}), static_cast<std::size_t>(0));
    TYST_EXPECT_EQ(model.predict_stop_stage({{1.0}, {1.0}}), static_cast<std::size_t>(2));
    TYST_EXPECT_LT(model.empirical_cost(features, costs), always_pass_cost(costs));
}

TYST_TEST(CatenarySupportVectorMachineTest, ExposesMarginScores) {
    CatenarySupportVectorMachine model;
    model.fit(make_pipeline_features(), make_pipeline_costs());

    const auto rejected_scores = model.decision_scores({{-1.0}, {-1.0}});
    const auto accepted_scores = model.decision_scores({{1.0}, {1.0}});

    TYST_EXPECT_EQ(rejected_scores.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_TRUE(rejected_scores[0] < 0.0);
    TYST_EXPECT_TRUE(accepted_scores[0] >= 0.0);
}