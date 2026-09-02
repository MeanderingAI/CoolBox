#include <ucb_agent.h>
#include <iostream>
#include <random>
#include <cmath>

UCBAgent::UCBAgent(const std::vector<double>& true_probs, double c) :
    BanditAgent(true_probs),
    c_(c),
    total_pulls_(0) {}

void UCBAgent::choose_and_pull() {
    total_pulls_++;
    int chosen_arm_index = get_best_ucb_index();

    double reward = arms_[chosen_arm_index].pull();
    arms_[chosen_arm_index].update(reward);
}

int UCBAgent::get_best_ucb_index() {
    double best_ucb_value = -1.0;
    int best_bandit_index = -1;

    for (size_t i = 0; i < arms_.size(); ++i) {
        if (arms_[i].get_pull_count() == 0) {
            return i; // Pull each arm at least once
        }

        // The core UCB formula
        double ucb_value = arms_[i].get_estimated_prob() + c_ * std::sqrt(std::log(total_pulls_) / arms_[i].get_pull_count());

        if (ucb_value > best_ucb_value) {
            best_ucb_value = ucb_value;
            best_bandit_index = i;
        }
    }
    return best_bandit_index;
}