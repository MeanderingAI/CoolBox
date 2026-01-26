#ifndef BANDIT_AGENT_H
#define BANDIT_AGENT_H

#include "ml/multi_arm_bandit/bandit_arm.h"
#include "ml/multi_arm_bandit/simulation_result.h"
#include <vector>

class BanditAgent {
public:
    BanditAgent(const std::vector<double>& true_probs);
    virtual ~BanditAgent() = default; // Virtual destructor is crucial for polymorphism

    void run_simulation(int num_steps);
    SimulationResult get_results() const;
    BanditArm& get_bandit(int index);

protected:
    // Pure virtual function that must be implemented by derived classes
    virtual void choose_and_pull() = 0;

    std::vector<BanditArm> arms_;
};

#endif // BANDIT_AGENT_H