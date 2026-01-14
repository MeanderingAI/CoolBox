#include "ML/multi_arm_bandit/bandit_agent.h"

BanditAgent::BanditAgent(const std::vector<double>& true_probs) {
    for (double prob : true_probs) {
        arms_.emplace_back(prob);
    }
}

void BanditAgent::run_simulation(int num_steps) {
    for (int i = 0; i < num_steps; ++i) {
        choose_and_pull();
    }
}

SimulationResult BanditAgent::get_results() const {
    SimulationResult result;
    for (const auto& arm : arms_) {
        result.bandit_results.push_back({
            arm.get_true_prob(),
            arm.get_estimated_prob(),
            arm.get_pull_count()
        });
    }
    return result;
}

BanditArm& BanditAgent::get_bandit(int index) {
    return arms_.at(index);
}