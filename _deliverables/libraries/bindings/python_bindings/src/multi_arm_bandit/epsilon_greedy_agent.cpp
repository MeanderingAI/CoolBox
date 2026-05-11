#include <epsilon_greedy_agent.h>
#include <random>
#include <iostream>

EpsilonGreedyAgent::EpsilonGreedyAgent(const std::vector<double>& true_probs, double epsilon, long long seed)
    : BanditAgent(true_probs), epsilon_(epsilon), gen(seed) {}

void EpsilonGreedyAgent::choose_and_pull() {
    std::uniform_real_distribution<> dis(0.0, 1.0);

    int chosen_arm_index;
    if (dis(gen) < epsilon_) {
        // Exploration: Choose a random arm
        chosen_arm_index = std::uniform_int_distribution<>(0, arms_.size() - 1)(gen);
    } else {
        // Exploitation: Choose the best-known arm
        chosen_arm_index = get_best_arm_index();
    }

    double reward = arms_[chosen_arm_index].pull();
    arms_[chosen_arm_index].update(reward);
}

int EpsilonGreedyAgent::get_best_arm_index() const {
    double max_est_prob = -1.0;
    int best_index = -1;
    for (size_t i = 0; i < arms_.size(); ++i) {
        if (arms_[i].get_estimated_prob() > max_est_prob) {
            max_est_prob = arms_[i].get_estimated_prob();
            best_index = i;
        }
    }
    return best_index;
}