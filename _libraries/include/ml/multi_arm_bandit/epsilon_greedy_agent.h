#ifndef EPSILON_GREEDY_AGENT_H
#define EPSILON_GREEDY_AGENT_H

#include "ml/multi_arm_bandit/bandit_agent.h"
#include <random>

class EpsilonGreedyAgent : public BanditAgent {
public:
    EpsilonGreedyAgent(const std::vector<double>& true_probs, double epsilon, long long seed = 0);

protected:
    void choose_and_pull() override;

private:
    int get_best_arm_index() const;
    double epsilon_;
    std::mt19937 gen;
};

#endif // EPSILON_GREEDY_AGENT_H