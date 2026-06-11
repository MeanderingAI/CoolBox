#include "tyst_framework.hpp"

#include "hill_climbing.h"

#include <memory>
#include <vector>

using namespace opt;

TEST(HillClimbingTest, ImprovesObjectiveAndFindsNearOptimum) {
    HillClimbing::Config cfg;
    cfg.dimensions = 2;
    cfg.max_iterations = 250;
    cfg.max_restarts = 6;
    cfg.seed = 99;

    HillClimbing search(cfg);
    auto objective = [](const std::vector<double>& x) {
        const double dx = x[0] - 2.0;
        const double dy = x[1] - 1.0;
        return dx * dx + dy * dy;
    };

    const std::vector<double> start = {-8.0, -8.0};
    const double start_score = objective(start);
    const std::vector<double> best = search.optimize(objective, start);

    ASSERT_EQ(best.size(), static_cast<std::size_t>(2));
    EXPECT_LT(search.best_score(), start_score);
    EXPECT_LT(search.best_score(), 0.4);
}

TEST(HillClimbingTest, SupportsUnifiedOptimizationInterface) {
    HillClimbing::Config cfg;
    cfg.dimensions = 2;
    cfg.seed = 1234;

    std::unique_ptr<OptimizationAlgorithm> algorithm = std::make_unique<HillClimbing>(cfg);
    auto objective = [](const std::vector<double>& x) {
        const double dx = x[0] - 1.0;
        const double dy = x[1] - 1.0;
        return dx * dx + dy * dy;
    };

    const auto best = algorithm->optimize(objective, {-5.0, 3.0});
    ASSERT_EQ(best.size(), static_cast<std::size_t>(2));
    EXPECT_LT(algorithm->best_score(), objective({-5.0, 3.0}));
}
