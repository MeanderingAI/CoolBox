#ifndef GENETIC_SEARCH_H
#define GENETIC_SEARCH_H

#include "optimization_algorithm.h"

#include <vector>

namespace opt {

class GeneticSearch : public OptimizationAlgorithm {
public:
    struct Config {
        Config()
            : dimensions(2),
              population_size(40),
              generations(80),
              tournament_size(3),
              crossover_rate(0.85),
              mutation_rate(0.12),
              mutation_sigma(0.20),
              lower_bound(-10.0),
              upper_bound(10.0),
              seed(42) {}

        int dimensions;
        int population_size;
        int generations;
        int tournament_size;
        double crossover_rate;
        double mutation_rate;
        double mutation_sigma;
        double lower_bound;
        double upper_bound;
        unsigned int seed;
    };

    explicit GeneticSearch(const Config& config = Config());

    std::vector<double> optimize(
        const ObjectiveFunction& objective,
        const std::vector<double>& initial_state) override;

    std::vector<double> optimize(const ObjectiveFunction& objective);

    const std::vector<double>& best_solution() const override;
    double best_score() const override;

private:
    Config config_;
    std::vector<double> best_solution_;
    double best_score_;
};

} // namespace opt

#endif // GENETIC_SEARCH_H
