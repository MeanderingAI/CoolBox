#ifndef OPTIMIZATION_FACTORY_H
#define OPTIMIZATION_FACTORY_H

#include "optimization_algorithm.h"

#include "genetic_search.h"
#include "hill_climbing.h"
#include "simulated_annealing.h"

#include <memory>
#include <string>

namespace opt {

enum class OptimizationType {
    GeneticSearch,
    SimulatedAnnealing,
    HillClimbing
};

OptimizationType optimization_type_from_string(const std::string& value);
std::string to_string(OptimizationType value);

std::unique_ptr<OptimizationAlgorithm> create_optimizer(OptimizationType type);
std::unique_ptr<OptimizationAlgorithm> create_optimizer(const GeneticSearch::Config& config);
std::unique_ptr<OptimizationAlgorithm> create_optimizer(const SimulatedAnnealing::Config& config);
std::unique_ptr<OptimizationAlgorithm> create_optimizer(const HillClimbing::Config& config);

} // namespace opt

#endif // OPTIMIZATION_FACTORY_H
