#include "bandit_agent.h"

BanditAgent::BanditAgent(const std::vector<double>& true_probs) {
    for (double p : true_probs) {
        arms_.emplace_back(p);
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
        BanditStats stats;
        stats.true_probability = arm.get_true_prob();
        stats.estimated_probability = arm.get_estimated_prob();
        stats.times_pulled = arm.get_pull_count();
        result.bandit_results.push_back(stats);
    }
    return result;
}

BanditArm& BanditAgent::get_bandit(int index) {
    return arms_[index];
}
