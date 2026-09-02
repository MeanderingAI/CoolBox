#ifndef HMM_FLARE_ADAPTER_H
#define HMM_FLARE_ADAPTER_H

#include "flare_mcmc.h"
#include "hidden_markov_model.h"

#include <cstddef>
#include <string>
#include <vector>

struct HmmFlarePrior {
    double initial_concentration = 1.0;
    double transition_concentration = 1.0;
    double emission_concentration = 1.0;
};

struct HmmFlareProblem {
    int states = 0;
    int observations = 0;
    std::vector<std::vector<int>> observation_sequences;
    HmmFlarePrior prior;
};

std::size_t hmm_flare_parameter_count(int states, int observations);
HMM hmm_from_flare_state(const ml::FlareState& parameters, int states, int observations);
double hmm_flare_log_prior(const ml::FlareState& parameters,
                           int states,
                           int observations,
                           const HmmFlarePrior& prior);
ml::FlareLayer make_hmm_flare_layer(const std::string& name,
                                    const HmmFlareProblem& problem,
                                    std::size_t observation_stride = 1,
                                    double tuning_omega = 0.0);
std::vector<ml::FlareLayer> make_hmm_flare_layers(const HmmFlareProblem& problem,
                                                  const std::vector<std::size_t>& observation_strides);

#endif // HMM_FLARE_ADAPTER_H