#include "ucb_agent.h"
#include <cmath>
#include <limits>

UCBAgent::UCBAgent(const std::vector<double>& true_probs, double c)
    : BanditAgent(true_probs), c_(c), total_pulls_(0) {}

void UCBAgent::choose_and_pull() {
    int chosen = get_best_ucb_index();
    double reward = arms_[chosen].pull();
    arms_[chosen].update(reward);
    total_pulls_++;
}

int UCBAgent::get_best_ucb_index() {
    // First, ensure all arms pulled at least once
    for (size_t i = 0; i < arms_.size(); ++i) {
        if (arms_[i].get_pull_count() == 0) return static_cast<int>(i);
    }

    int best = 0;
    double best_ucb = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < arms_.size(); ++i) {
        double ucb = arms_[i].get_estimated_prob() +
                     c_ * std::sqrt(std::log(static_cast<double>(total_pulls_)) / arms_[i].get_pull_count());
        if (ucb > best_ucb) {
            best_ucb = ucb;
            best = static_cast<int>(i);
        }
    }
    return best;
}
