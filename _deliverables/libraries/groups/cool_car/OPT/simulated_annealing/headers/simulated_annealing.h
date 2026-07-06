#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include "optimization_algorithm.h"

#include <vector>

namespace opt {

class SimulatedAnnealing : public OptimizationAlgorithm {
public:
    struct Config {
        Config()
            : dimensions(2),
              iterations_per_temperature(40),
              initial_temperature(3.0),
              cooling_rate(0.95),
              minimum_temperature(1e-4),
              step_sigma(0.3),
              lower_bound(-10.0),
              upper_bound(10.0),
              seed(123) {}

        int dimensions;
        int iterations_per_temperature;
        double initial_temperature;
        double cooling_rate;
        double minimum_temperature;
        double step_sigma;
        double lower_bound;
        double upper_bound;
        unsigned int seed;
    };

    explicit SimulatedAnnealing(const Config& config = Config());

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

#endif // SIMULATED_ANNEALING_H
