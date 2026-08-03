#include "tyst_framework.hpp"

#include "quantum_control_bo.h"

#include <cmath>

using namespace opt;

namespace {

double transition_probability(const std::vector<double>& point) {
    const double frequency_error = point[0] - 1.2;
    const double amplitude_error = point[1] - 0.08;
    return std::exp(-40.0 * frequency_error * frequency_error - 800.0 * amplitude_error * amplitude_error);
}

double minimization_objective(const std::vector<double>& point) {
    return -transition_probability(point);
}

} // namespace

TYST_TEST(QuantumControlBayesianOptimizerTest, CropPolicyContractsAroundBestFrequency) {
    QuantumControlBayesianOptimizer::Config config;
    config.policy = QuantumControlSearchPolicy::Crop;
    config.bounds.frequency_center = 1.0;
    config.bounds.initial_frequency_ratio = 0.5;
    config.initial_samples = 4;
    config.iterations = 8;
    config.acquisition_candidates = 32;
    config.seed = 9;

    QuantumControlBayesianOptimizer optimizer(config);
    const auto best = optimizer.optimize(minimization_objective, {1.15, 0.08});
    const auto bounds = optimizer.current_bounds();

    TYST_EXPECT_TRUE(best.size() == 2);
    TYST_EXPECT_TRUE(optimizer.best_probability() > 0.5);
    TYST_EXPECT_TRUE(bounds.current_frequency_ratio < config.bounds.initial_frequency_ratio);
    TYST_EXPECT_NEAR(bounds.frequency_center, best[0], 1e-12);
}

TYST_TEST(QuantumControlBayesianOptimizerTest, WarpPolicyKeepsGlobalBoundsAndImprovesProbability) {
    QuantumControlBayesianOptimizer::Config config;
    config.policy = QuantumControlSearchPolicy::Warp;
    config.bounds.frequency_center = 1.0;
    config.bounds.initial_frequency_ratio = 0.6;
    config.initial_samples = 6;
    config.iterations = 16;
    config.acquisition_candidates = 48;
    config.seed = 12;

    QuantumControlBayesianOptimizer optimizer(config);
    const double start_probability = transition_probability({0.55, 0.14});
    const auto best = optimizer.optimize(minimization_objective, {0.55, 0.14});
    const auto bounds = optimizer.current_bounds();

    TYST_EXPECT_TRUE(best.size() == 2);
    TYST_EXPECT_TRUE(optimizer.best_probability() > start_probability);
    TYST_EXPECT_NEAR(bounds.frequency_center, config.bounds.frequency_center, 1e-12);
    TYST_EXPECT_NEAR(bounds.current_frequency_ratio, config.bounds.initial_frequency_ratio, 1e-12);
}

TYST_TEST(QuantumControlBayesianOptimizerTest, WarpMappingRoundTripsFrequency) {
    QuantumControlBayesianOptimizer::Config config;
    config.policy = QuantumControlSearchPolicy::Warp;
    config.initial_samples = 4;
    config.iterations = 4;
    config.acquisition_candidates = 16;

    QuantumControlBayesianOptimizer optimizer(config);
    optimizer.optimize([](const std::vector<double>& point) {
        return -0.8 * transition_probability(point);
    }, {1.2, 0.08});
    const std::vector<double> point = {0.9, 0.05};
    const std::vector<double> round_trip = optimizer.unwarp_frequency_point(optimizer.warp_frequency_point(point));

    TYST_EXPECT_NEAR(round_trip[0], point[0], 1e-8);
    TYST_EXPECT_NEAR(round_trip[1], point[1], 1e-12);
}