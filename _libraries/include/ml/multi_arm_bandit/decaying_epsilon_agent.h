#ifndef DECAYING_EPSILON_GREEDY_AGENT_H
#define DECAYING_EPSILON_GREEDY_AGENT_H

#include "ml/multi_arm_bandit/bandit_agent.h"
#include <random>

class DecayingEpsilonGreedyAgent : public BanditAgent {
public:
    DecayingEpsilonGreedyAgent(const std::vector<double>& true_probs, double initial_epsilon, double decay_rate, long long seed);

protected:
    void choose_and_pull() override;

private:
    double get_current_epsilon() const;
    int get_best_arm_index() const;
    
    double initial_epsilon_;
    double decay_rate_;
    int total_pulls_;
    std::mt19937 gen;
};

#endif // DECAYING_EPSILON_GREEDY_AGENT_H