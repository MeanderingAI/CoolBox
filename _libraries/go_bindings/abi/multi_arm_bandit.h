#ifndef COOLBOX_GO_BINDINGS_ABI_MULTI_ARM_BANDIT_H
#define COOLBOX_GO_BINDINGS_ABI_MULTI_ARM_BANDIT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxBanditArmModel CoolBoxBanditArmModel;
typedef struct CoolBoxBanditAgentModel CoolBoxBanditAgentModel;

CoolBoxBanditArmModel* coolbox_create_bandit_arm(
    double true_reward_prob,
    char** error_message);

int coolbox_bandit_arm_pull(
    CoolBoxBanditArmModel* model,
    double* out_reward,
    char** error_message);

int coolbox_bandit_arm_update(
    CoolBoxBanditArmModel* model,
    double reward,
    char** error_message);

double coolbox_bandit_arm_estimated_prob(const CoolBoxBanditArmModel* model);
int coolbox_bandit_arm_pull_count(const CoolBoxBanditArmModel* model);
double coolbox_bandit_arm_true_prob(const CoolBoxBanditArmModel* model);

void coolbox_free_bandit_arm(CoolBoxBanditArmModel* model);

CoolBoxBanditAgentModel* coolbox_create_epsilon_greedy_agent(
    const double* true_probs,
    size_t count,
    double epsilon,
    double unused,
    long long seed,
    char** error_message);

CoolBoxBanditAgentModel* coolbox_create_ucb_agent(
    const double* true_probs,
    size_t count,
    double c,
    double unused,
    long long seed,
    char** error_message);

CoolBoxBanditAgentModel* coolbox_create_thompson_sampling_agent(
    const double* true_probs,
    size_t count,
    double unused1,
    double unused2,
    long long seed,
    char** error_message);

CoolBoxBanditAgentModel* coolbox_create_decaying_epsilon_greedy_agent(
    const double* true_probs,
    size_t count,
    double initial_epsilon,
    double decay_rate,
    long long seed,
    char** error_message);

int coolbox_bandit_agent_run_simulation(
    CoolBoxBanditAgentModel* model,
    int num_steps,
    char** error_message);

size_t coolbox_bandit_agent_arm_count(const CoolBoxBanditAgentModel* model);

int coolbox_bandit_agent_get_results(
    const CoolBoxBanditAgentModel* model,
    double* out_true_probs,
    double* out_estimated_probs,
    int* out_pull_counts,
    size_t count,
    char** error_message);

void coolbox_free_bandit_agent(CoolBoxBanditAgentModel* model);

#ifdef __cplusplus
}
#endif

#endif
