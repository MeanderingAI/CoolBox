#ifndef THOMPSON_SAMPLING_AGENT_H
#define THOMPSON_SAMPLING_AGENT_H

#include "ML/multi_arm_bandit/bandit_agent.h"
#include <random>

class ThompsonSamplingAgent : public BanditAgent {
public:
    ThompsonSamplingAgent(const std::vector<double>& true_probs, long long seed);

protected:
    void choose_and_pull() override;

private:
    int get_best_sampled_index();

    std::vector<double> alphas_;
    std::vector<double> betas_;
    std::mt19937 gen;
};

#endif // THOMPSON_SAMPLING_AGENT_H