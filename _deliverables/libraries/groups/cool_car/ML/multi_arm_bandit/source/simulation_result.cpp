#include "simulation_result.h"
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const SimulationResult& result) {
    os << "Simulation Results:" << std::endl;
    os << std::setw(10) << "Arm" << std::setw(15) << "True Prob"
       << std::setw(15) << "Est. Prob" << std::setw(15) << "Pulls" << std::endl;
    os << std::string(55, '-') << std::endl;
    for (size_t i = 0; i < result.bandit_results.size(); ++i) {
        const auto& b = result.bandit_results[i];
        os << std::setw(10) << i
           << std::setw(15) << std::fixed << std::setprecision(4) << b.true_probability
           << std::setw(15) << std::fixed << std::setprecision(4) << b.estimated_probability
           << std::setw(15) << b.times_pulled << std::endl;
    }
    return os;
}
