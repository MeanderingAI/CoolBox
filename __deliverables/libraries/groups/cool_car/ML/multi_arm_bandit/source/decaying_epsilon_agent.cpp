#include "decaying_epsilon_agent.h"
#include <cmath>

DecayingEpsilonGreedyAgent::DecayingEpsilonGreedyAgent(
    const std::vector<double>& true_probs, double initial_epsilon, double decay_rate, long long seed)
    : BanditAgent(true_probs), initial_epsilon_(initial_epsilon),
      decay_rate_(decay_rate), total_pulls_(0), gen(seed) {}

void DecayingEpsilonGreedyAgent::choose_and_pull() {
    double epsilon = get_current_epsilon();
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    int chosen;
    if (dist(gen) < epsilon) {
        std::uniform_int_distribution<int> arm_dist(0, static_cast<int>(arms_.size()) - 1);
        chosen = arm_dist(gen);
    } else {
        chosen = get_best_arm_index();
    }
    double reward = arms_[chosen].pull();
    arms_[chosen].update(reward);
    total_pulls_++;
}

double DecayingEpsilonGreedyAgent::get_current_epsilon() const {
    return initial_epsilon_ * std::exp(-decay_rate_ * total_pulls_);
}

int DecayingEpsilonGreedyAgent::get_best_arm_index() const {
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
