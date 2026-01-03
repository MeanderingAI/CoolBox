#ifndef UCB_AGENT_H
#define UCB_AGENT_H

#include "ml/multi_arm_bandit/bandit_agent.h"
#include <vector>

class UCBAgent : public BanditAgent {
public:
    UCBAgent(const std::vector<double>& true_probs, double c);

protected:
    void choose_and_pull() override;

private:
    int get_best_ucb_index();
    double c_;
    int total_pulls_;
};

#endif // UCB_AGENT_H