#include <simulation_result.h>

std::ostream& operator<<(std::ostream& os, const SimulationResult& result) {
    os << "Simulation finished." << std::endl;
    for (size_t i = 0; i < result.bandit_results.size(); ++i) {
        os << "Arm " << i << ":" << std::endl;
        os << "  True Probability: " << result.bandit_results[i].true_probability << std::endl;
        os << "  Estimated Probability: " << result.bandit_results[i].estimated_probability << std::endl;
        os << "  Times Pulled: " << result.bandit_results[i].times_pulled << std::endl;
    }
    return os;
}