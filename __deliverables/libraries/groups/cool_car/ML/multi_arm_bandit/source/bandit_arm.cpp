#include "bandit_arm.h"
#include <random>

static std::mt19937& get_rng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

BanditArm::BanditArm(double true_reward_prob)
    : true_prob(true_reward_prob), estimated_prob(0.0), pull_count(0) {}

double BanditArm::pull() {
    std::bernoulli_distribution dist(true_prob);
    return dist(get_rng()) ? 1.0 : 0.0;
}

void BanditArm::update(double reward) {
    pull_count++;
    // Incremental mean update
    estimated_prob += (reward - estimated_prob) / pull_count;
}

double BanditArm::get_estimated_prob() const { return estimated_prob; }
int BanditArm::get_pull_count() const { return pull_count; }
double BanditArm::get_true_prob() const { return true_prob; }
