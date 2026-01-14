#include "ML/multi_arm_bandit/bandit_arm.h"
#include <random>

BanditArm::BanditArm(double true_reward_prob) :
    true_prob(true_reward_prob),
    estimated_prob(0.0),
    pull_count(0) {}

double BanditArm::pull() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution d(true_prob);
    return d(gen);
}

void BanditArm::update(double reward) {
    pull_count++;
    estimated_prob = (estimated_prob * (pull_count - 1) + reward) / pull_count;
}

double BanditArm::get_estimated_prob() const { return estimated_prob; }
int BanditArm::get_pull_count() const { return pull_count; }
double BanditArm::get_true_prob() const { return true_prob; }