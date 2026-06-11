#include "optimization_factory.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {

std::string lowercase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

} // namespace

namespace opt {

OptimizationType optimization_type_from_string(const std::string& value) {
    const std::string key = lowercase(value);

    if (key == "genetic" || key == "genetic_search" || key == "ga") {
        return OptimizationType::GeneticSearch;
    }
    if (key == "simulated_annealing" || key == "annealing" || key == "sa") {
        return OptimizationType::SimulatedAnnealing;
    }
    if (key == "hill_climbing" || key == "hillclimbing" || key == "hc") {
        return OptimizationType::HillClimbing;
    }

    throw std::invalid_argument("Unknown optimization type: " + value);
}

std::string to_string(OptimizationType value) {
    switch (value) {
        case OptimizationType::GeneticSearch:
            return "genetic_search";
        case OptimizationType::SimulatedAnnealing:
            return "simulated_annealing";
        case OptimizationType::HillClimbing:
            return "hill_climbing";
        default:
            throw std::invalid_argument("Unsupported optimization type");
    }
}

std::unique_ptr<OptimizationAlgorithm> create_optimizer(OptimizationType type) {
    switch (type) {
        case OptimizationType::GeneticSearch:
            return std::make_unique<GeneticSearch>();
        case OptimizationType::SimulatedAnnealing:
            return std::make_unique<SimulatedAnnealing>();
        case OptimizationType::HillClimbing:
            return std::make_unique<HillClimbing>();
        default:
            throw std::invalid_argument("Unsupported optimization type");
    }
}

std::unique_ptr<OptimizationAlgorithm> create_optimizer(const GeneticSearch::Config& config) {
    return std::make_unique<GeneticSearch>(config);
}

std::unique_ptr<OptimizationAlgorithm> create_optimizer(const SimulatedAnnealing::Config& config) {
    return std::make_unique<SimulatedAnnealing>(config);
}

std::unique_ptr<OptimizationAlgorithm> create_optimizer(const HillClimbing::Config& config) {
    return std::make_unique<HillClimbing>(config);
}

} // namespace opt
