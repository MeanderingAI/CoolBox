#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxLinearRegressionModel CoolBoxLinearRegressionModel;
typedef struct CoolBoxDecisionTreeModel CoolBoxDecisionTreeModel;
typedef struct CoolBoxBayesianNetworkModel CoolBoxBayesianNetworkModel;
typedef struct CoolBoxHMMModel CoolBoxHMMModel;
typedef struct CoolBoxPCAModel CoolBoxPCAModel;
typedef struct CoolBoxSVMModel CoolBoxSVMModel;
typedef struct CoolBoxBanditArmModel CoolBoxBanditArmModel;
typedef struct CoolBoxBanditAgentModel CoolBoxBanditAgentModel;

CoolBoxLinearRegressionModel* coolbox_fit_linear_regression(
    const double* x_flat,
    size_t rows,
    size_t cols,
    const double* y,
    const char* method,
    uint32_t iterations,
    double learning_rate,
    char** error_message);

int coolbox_predict_linear_regression(
    const CoolBoxLinearRegressionModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    double* out_predictions,
    char** error_message);

size_t coolbox_linear_regression_feature_count(
    const CoolBoxLinearRegressionModel* model);

double coolbox_linear_regression_intercept(
    const CoolBoxLinearRegressionModel* model);

double coolbox_linear_regression_weight_at(
    const CoolBoxLinearRegressionModel* model,
    size_t index);

const char* coolbox_linear_regression_method_name(
    const CoolBoxLinearRegressionModel* model);

void coolbox_free_linear_regression(
    CoolBoxLinearRegressionModel* model);

CoolBoxDecisionTreeModel* coolbox_create_decision_tree(
    const char* criterion,
    char** error_message);

int coolbox_decision_tree_fit(
    CoolBoxDecisionTreeModel* model,
    const int* x_flat,
    size_t rows,
    size_t cols,
    const int* y,
    int max_depth,
    char** error_message);

int coolbox_decision_tree_predict(
    const CoolBoxDecisionTreeModel* model,
    const int* sample,
    size_t feature_count,
    int* out_label,
    char** error_message);

void coolbox_free_decision_tree(CoolBoxDecisionTreeModel* model);

CoolBoxBayesianNetworkModel* coolbox_create_bayesian_network(void);

int coolbox_bayesian_network_add_node(
    CoolBoxBayesianNetworkModel* model,
    const char* name,
    const char* const* states,
    size_t state_count,
    int* out_node_id,
    char** error_message);

int coolbox_bayesian_network_add_edge(
    CoolBoxBayesianNetworkModel* model,
    int parent_id,
    int child_id,
    char** error_message);

int coolbox_bayesian_network_set_cpt(
    CoolBoxBayesianNetworkModel* model,
    int node_id,
    const double* values,
    size_t value_count,
    char** error_message);

size_t coolbox_bayesian_network_num_nodes(
    const CoolBoxBayesianNetworkModel* model);

int coolbox_bayesian_network_get_node_id(
    const CoolBoxBayesianNetworkModel* model,
    const char* name,
    int* out_node_id,
    char** error_message);

int coolbox_bayesian_network_get_node_state_count(
    const CoolBoxBayesianNetworkModel* model,
    int node_id,
    size_t* out_state_count,
    char** error_message);

int coolbox_bayesian_network_query(
    const CoolBoxBayesianNetworkModel* model,
    int query_node,
    const int* evidence_node_ids,
    const int* evidence_state_ids,
    size_t evidence_count,
    double* out_distribution,
    size_t distribution_count,
    char** error_message);

void coolbox_free_bayesian_network(CoolBoxBayesianNetworkModel* model);

CoolBoxHMMModel* coolbox_create_hmm(
    size_t states,
    size_t observations,
    char** error_message);

int coolbox_hmm_set_initial_probabilities(
    CoolBoxHMMModel* model,
    const double* values,
    size_t count,
    char** error_message);

int coolbox_hmm_set_transition_matrix(
    CoolBoxHMMModel* model,
    const double* values,
    size_t rows,
    size_t cols,
    char** error_message);

int coolbox_hmm_set_emission_matrix(
    CoolBoxHMMModel* model,
    const double* values,
    size_t rows,
    size_t cols,
    char** error_message);

int coolbox_hmm_get_initial_probabilities(
    const CoolBoxHMMModel* model,
    double* out_values,
    size_t count,
    char** error_message);

int coolbox_hmm_get_transition_matrix(
    const CoolBoxHMMModel* model,
    double* out_values,
    size_t rows,
    size_t cols,
    char** error_message);

int coolbox_hmm_get_emission_matrix(
    const CoolBoxHMMModel* model,
    double* out_values,
    size_t rows,
    size_t cols,
    char** error_message);

int coolbox_hmm_log_likelihood(
    const CoolBoxHMMModel* model,
    const int* observations,
    size_t observation_count,
    double* out_value,
    char** error_message);

int coolbox_hmm_get_most_likely_states(
    const CoolBoxHMMModel* model,
    const int* observations,
    size_t observation_count,
    int* out_states,
    size_t state_count,
    char** error_message);

int coolbox_hmm_train(
    CoolBoxHMMModel* model,
    const int* sequences_flat,
    const size_t* sequence_lengths,
    size_t sequence_count,
    int max_iterations,
    double tolerance,
    double smoothing_factor,
    uint32_t seed,
    char** error_message);

void coolbox_free_hmm(CoolBoxHMMModel* model);

CoolBoxPCAModel* coolbox_create_pca(
    int n_components,
    int center,
    int scale,
    char** error_message);

int coolbox_pca_fit(
    CoolBoxPCAModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    char** error_message);

int coolbox_pca_transform(
    const CoolBoxPCAModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_fit_transform(
    CoolBoxPCAModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    double* out_values,
    size_t out_count,
    size_t* out_cols,
    char** error_message);

int coolbox_pca_inverse_transform(
    const CoolBoxPCAModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_components(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_explained_variance(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_explained_variance_ratio(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_singular_values(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_mean(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

int coolbox_pca_get_scale(
    const CoolBoxPCAModel* model,
    double* out_values,
    size_t out_count,
    char** error_message);

size_t coolbox_pca_component_count(const CoolBoxPCAModel* model);
size_t coolbox_pca_feature_count(const CoolBoxPCAModel* model);
int coolbox_pca_is_fitted(const CoolBoxPCAModel* model);

void coolbox_free_pca(CoolBoxPCAModel* model);

CoolBoxSVMModel* coolbox_create_svm(
    const char* kernel_type,
    double param1,
    double param2,
    int param3,
    char** error_message);

int coolbox_svm_fit(
    CoolBoxSVMModel* model,
    const double* x_flat,
    size_t rows,
    size_t cols,
    const double* y,
    size_t y_count,
    char** error_message);

int coolbox_svm_predict(
    const CoolBoxSVMModel* model,
    const double* sample,
    size_t feature_count,
    double* out_value,
    char** error_message);

void coolbox_free_svm(CoolBoxSVMModel* model);

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

void coolbox_free_string(char* value);

#ifdef __cplusplus
}
#endif
