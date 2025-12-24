#include <decaying_epsilon_agent.h>
#include <iostream>

DecayingEpsilonGreedyAgent::DecayingEpsilonGreedyAgent(const std::vector<double>& true_probs, double initial_epsilon, double decay_rate, long long seed)
    : BanditAgent(true_probs), 
      initial_epsilon_(initial_epsilon),
      decay_rate_(decay_rate),
      total_pulls_(0),
      gen(seed) {}

void DecayingEpsilonGreedyAgent::choose_and_pull() {
    total_pulls_++;
    double current_epsilon = get_current_epsilon();
    
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    int chosen_arm_index;
    if (dis(gen) < current_epsilon) {
        // Exploration: Choose a random arm
        chosen_arm_index = std::uniform_int_distribution<>(0, arms_.size() - 1)(gen);
    } else {
        // Exploitation: Choose the best-known arm
        chosen_arm_index = get_best_arm_index();
    }
    
    double reward = arms_[chosen_arm_index].pull();
    arms_[chosen_arm_index].update(reward);
}

double DecayingEpsilonGreedyAgent::get_current_epsilon() const {
    // A simple decay schedule: epsilon decreases with the number of pulls
    // The epsilon value won't go below a certain threshold to ensure some exploration continues
    return initial_epsilon_ / (1.0 + decay_rate_ * total_pulls_);
}

int DecayingEpsilonGreedyAgent::get_best_arm_index() const {
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