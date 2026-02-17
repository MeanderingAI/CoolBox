#ifndef SIMULATION_RESULT_H
#define SIMULATION_RESULT_H

#include <vector>
#include <iostream>

struct BanditStats {
    double true_probability;
    double estimated_probability;
    int times_pulled;
};

struct SimulationResult {
    std::vector<BanditStats> bandit_results;
    
    // Friend declaration for the overloaded stream insertion operator
    friend std::ostream& operator<<(std::ostream& os, const SimulationResult& result);
};

#endif // SIMULATION_RESULT_H