#ifndef COOLBOX_GO_BINDINGS_ABI_BAYESIAN_NETWORK_H
#define COOLBOX_GO_BINDINGS_ABI_BAYESIAN_NETWORK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxBayesianNetworkModel CoolBoxBayesianNetworkModel;

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

#ifdef __cplusplus
}
#endif

#endif
