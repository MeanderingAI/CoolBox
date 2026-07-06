#ifndef HILL_CLIMBING_H
#define HILL_CLIMBING_H

#include "optimization_algorithm.h"

#include <vector>

namespace opt {

class HillClimbing : public OptimizationAlgorithm {
public:
    struct Config {
        Config()
            : dimensions(2),
              max_iterations(200),
              neighbours_per_iteration(24),
              max_restarts(4),
              step_sigma(0.25),
              lower_bound(-10.0),
              upper_bound(10.0),
              seed(2026) {}

        int dimensions;
        int max_iterations;
        int neighbours_per_iteration;
        int max_restarts;
        double step_sigma;
        double lower_bound;
        double upper_bound;
        unsigned int seed;
    };

    explicit HillClimbing(const Config& config = Config());

    std::vector<double> optimize(
        const ObjectiveFunction& objective,
        const std::vector<double>& initial_state) override;

    const std::vector<double>& best_solution() const override;
    double best_score() const override;

private:
    Config config_;
    std::vector<double> best_solution_;
    double best_score_;
};

} // namespace opt

#endif // HILL_CLIMBING_H
