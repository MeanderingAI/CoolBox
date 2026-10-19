#include "ucb_agent.h"
#include <cmath>
#include <limits>
#include <stdexcept>

UCBAgent::UCBAgent(const std::vector<double>& true_probs, double c)
    : BanditAgent(true_probs), c_(c), total_pulls_(0) {
    if (true_probs.empty() || !std::isfinite(c) || c < 0) throw std::invalid_argument("UCB requires arms and a nonnegative exploration coefficient");
}

int UCBAgent::select_arm() { return get_best_ucb_index(); }

void UCBAgent::observe_reward(int arm, double reward) {
    if (arm < 0 || static_cast<std::size_t>(arm) >= arms_.size() || !std::isfinite(reward))
        throw std::invalid_argument("Invalid UCB observation");
    arms_[arm].update(reward);
    ++total_pulls_;
}

void UCBAgent::choose_and_pull() {
    int chosen = select_arm();
    double reward = arms_[chosen].pull();
    observe_reward(chosen, reward);
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
