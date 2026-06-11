#include "tyst_framework.hpp"

#include "optimization_factory.h"

#include <vector>

using namespace opt;

TYST_TEST(OptimizationFactoryTest, BuildsAlgorithmsFromEnumAndString) {
    const auto kind = optimization_type_from_string("ga");
    TYST_EXPECT_EQ(to_string(kind), std::string("genetic_search"));

    auto ga = create_optimizer(kind);
    auto sa = create_optimizer(OptimizationType::SimulatedAnnealing);
    auto hc = create_optimizer(OptimizationType::HillClimbing);

    TYST_EXPECT_TRUE(static_cast<bool>(ga));
    TYST_EXPECT_TRUE(static_cast<bool>(sa));
    TYST_EXPECT_TRUE(static_cast<bool>(hc));
}

TYST_TEST(OptimizationFactoryTest, PolymorphicRunViaFactory) {
    auto optimizer = create_optimizer(OptimizationType::HillClimbing);

    auto objective = [](const std::vector<double>& x) {
        const double dx = x[0] - 0.5;
        const double dy = x[1] + 1.0;
        return dx * dx + dy * dy;
    };

    const double start = objective({5.0, 5.0});
    const auto best = optimizer->optimize(objective, {5.0, 5.0});

    TYST_EXPECT_EQ(best.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_LT(optimizer->best_score(), start);
}
