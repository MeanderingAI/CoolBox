#ifndef COOLBOX_GO_BINDINGS_ABI_HMM_H
#define COOLBOX_GO_BINDINGS_ABI_HMM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoolBoxHMMModel CoolBoxHMMModel;

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

#ifdef __cplusplus
}
#endif

#endif
