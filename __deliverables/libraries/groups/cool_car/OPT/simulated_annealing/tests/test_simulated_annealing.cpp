#include "tyst_framework.hpp"

#include "simulated_annealing.h"

#include <vector>

using namespace opt;

TEST(SimulatedAnnealingTest, ReducesObjectiveFromPoorStart) {
    SimulatedAnnealing::Config cfg;
    cfg.dimensions = 2;
    cfg.iterations_per_temperature = 80;
    cfg.initial_temperature = 4.0;
    cfg.seed = 11;

    SimulatedAnnealing annealer(cfg);
    auto objective = [](const std::vector<double>& x) {
        const double dx = x[0] - 1.0;
        const double dy = x[1] + 2.0;
        return dx * dx + dy * dy;
    };

    const std::vector<double> start = {8.0, 8.0};
    const double start_score = objective(start);
    const std::vector<double> best = annealer.optimize(objective, start);

    ASSERT_EQ(best.size(), static_cast<std::size_t>(2));
    EXPECT_LT(annealer.best_score(), start_score);
    EXPECT_LT(annealer.best_score(), 1.0);
}
