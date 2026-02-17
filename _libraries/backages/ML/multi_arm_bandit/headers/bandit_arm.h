#ifndef BANDIT_ARM_H
#define BANDIT_ARM_H

class BanditArm {
public:
    BanditArm(double true_reward_prob);

    double pull();
    void update(double reward);

    double get_estimated_prob() const;
    int get_pull_count() const;
    double get_true_prob() const;

private:
    double true_prob;
    double estimated_prob;
    int pull_count;
};

#endif // BANDIT_ARM_H