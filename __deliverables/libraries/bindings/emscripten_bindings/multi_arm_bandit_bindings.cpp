#include <emscripten/bind.h>
#include "bandit_arm.h"
#include "bandit_agent.h"
#include "epsilon_greedy_agent.h"
#include "ucb_agent.h"
#include "thompson_sampling_agent.h"
#include "decaying_epsilon_agent.h"
#include "simulation_result.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(multi_arm_bandit_module) {
    class_<BanditArm>("BanditArm")
        .constructor<double>()
        .function("pull", &BanditArm::pull)
        .function("update", &BanditArm::update)
        .function("get_estimated_prob", &BanditArm::get_estimated_prob)
        .function("get_pull_count", &BanditArm::get_pull_count)
        .function("get_true_prob", &BanditArm::get_true_prob)
    ;

    class_<BanditAgent>("BanditAgent")
        .function("run_simulation", &BanditAgent::run_simulation)
    ;

    class_<EpsilonGreedyAgent, base<BanditAgent>>("EpsilonGreedyAgent")
        .constructor<std::vector<double>, double, long long>()
    ;

    class_<UCBAgent, base<BanditAgent>>("UCBAgent")
        .constructor<std::vector<double>, double>()
    ;

    class_<ThompsonSamplingAgent, base<BanditAgent>>("ThompsonSamplingAgent")
        .constructor<std::vector<double>, long long>()
    ;

    class_<DecayingEpsilonGreedyAgent, base<BanditAgent>>("DecayingEpsilonGreedyAgent")
        .constructor<std::vector<double>, double, double, long long>()
    ;

    register_vector<double>("VectorDouble");
}
