#include "tyst_framework.hpp"

#include "genetic_search.h"

#include <cmath>
#include <vector>

using namespace opt;

TEST(GeneticSearchTest, ImprovesTowardSphereMinimum) {
    GeneticSearch::Config cfg;
    cfg.dimensions = 3;
    cfg.population_size = 50;
    cfg.generations = 120;
    cfg.seed = 7;

    GeneticSearch search(cfg);
    auto objective = [](const std::vector<double>& x) {
        double sum = 0.0;
        for (double v : x) {
            sum += v * v;
        }
        return sum;
    };

    const std::vector<double> best = search.optimize(objective);
    ASSERT_EQ(best.size(), static_cast<std::size_t>(3));
    EXPECT_LT(search.best_score(), 0.5);
}
