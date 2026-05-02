#include <emscripten/bind.h>
#include "hidden_markov_model.h"

using namespace emscripten;

class HMMWrapper {
    HMM hmm_;
public:
    HMMWrapper(int states, int observations) : hmm_(states, observations) {}

    void train(const std::vector<std::vector<int>>& sequences, int max_iter) {
        hmm_.train(sequences, max_iter);
    }

    std::vector<int> get_most_likely_states(const std::vector<int>& observations) {
        return hmm_.get_most_likely_states(observations);
    }

    double log_likelihood(const std::vector<int>& observations) {
        return hmm_.log_likelihood(observations);
    }
};

EMSCRIPTEN_BINDINGS(hidden_markov_model_module) {
    register_vector<int>("VectorInt_HMM");
    register_vector<std::vector<int>>("VectorVectorInt_HMM");

    class_<HMMWrapper>("HMM")
        .constructor<int, int>()
        .function("train", &HMMWrapper::train)
        .function("get_most_likely_states", &HMMWrapper::get_most_likely_states)
        .function("log_likelihood", &HMMWrapper::log_likelihood)
    ;
}
