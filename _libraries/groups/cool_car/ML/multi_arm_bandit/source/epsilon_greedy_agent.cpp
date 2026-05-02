#include "epsilon_greedy_agent.h"
#include <algorithm>

EpsilonGreedyAgent::EpsilonGreedyAgent(const std::vector<double>& true_probs, double epsilon, long long seed)
    : BanditAgent(true_probs), epsilon_(epsilon), gen(seed) {}

void EpsilonGreedyAgent::choose_and_pull() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    int chosen;
    if (dist(gen) < epsilon_) {
        // Explore: pick random arm
        std::uniform_int_distribution<int> arm_dist(0, static_cast<int>(arms_.size()) - 1);
        chosen = arm_dist(gen);
    } else {
        // Exploit: pick best arm
        chosen = get_best_arm_index();
    }
    double reward = arms_[chosen].pull();
    arms_[chosen].update(reward);
}

int EpsilonGreedyAgent::get_best_arm_index() const {
    int best = 0;
    double best_val = arms_[0].get_estimated_prob();
    for (size_t i = 1; i < arms_.size(); ++i) {
        if (arms_[i].get_estimated_prob() > best_val) {
            best_val = arms_[i].get_estimated_prob();
            best = static_cast<int>(i);
        }
    }
    return best;
}
