#ifndef UCB_AGENT_H
#define UCB_AGENT_H

#include "bandit_agent.h"
#include <vector>

class UCBAgent : public BanditAgent {
public:
    UCBAgent(const std::vector<double>& true_probs, double c);
    int select_arm();
    void observe_reward(int arm, double reward);

protected:
    void choose_and_pull() override;

private:
    int get_best_ucb_index();
    double c_;
    int total_pulls_;
};

#endif // UCB_AGENT_H