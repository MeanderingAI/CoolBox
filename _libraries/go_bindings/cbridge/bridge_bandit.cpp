#include "../abi/multi_arm_bandit.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../packages/ML/multi_arm_bandit/headers/bandit_agent.h"
#include "../../packages/ML/multi_arm_bandit/headers/bandit_arm.h"
#include "../../packages/ML/multi_arm_bandit/headers/decaying_epsilon_agent.h"
#include "../../packages/ML/multi_arm_bandit/headers/epsilon_greedy_agent.h"
#include "../../packages/ML/multi_arm_bandit/headers/thompson_sampling_agent.h"
#include "../../packages/ML/multi_arm_bandit/headers/ucb_agent.h"

#include "../../packages/ML/multi_arm_bandit/source/bandit_arm.cpp"
#include "../../packages/ML/multi_arm_bandit/source/bandit_agent.cpp"
#include "../../packages/ML/multi_arm_bandit/source/epsilon_greedy_agent.cpp"
#include "../../packages/ML/multi_arm_bandit/source/ucb_agent.cpp"
#include "../../packages/ML/multi_arm_bandit/source/thompson_sampling_agent.cpp"
#include "../../packages/ML/multi_arm_bandit/source/decaying_epsilon_agent.cpp"

namespace {

void clear_error(char** error_message) {
    if (error_message != nullptr) {
        *error_message = nullptr;
    }
}

void set_error(char** error_message, const std::string& message) {
    if (error_message == nullptr) {
        return;
    }

    char* buffer = static_cast<char*>(std::malloc(message.size() + 1));
    if (buffer == nullptr) {
        *error_message = nullptr;
        return;
    }

    std::memcpy(buffer, message.c_str(), message.size() + 1);
    *error_message = buffer;
}

template <typename Callback>
int run_bridge_call(char** error_message, Callback&& callback) {
    clear_error(error_message);
    try {
        callback();
        return 1;
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return 0;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return 0;
    }
}

std::vector<double> probabilities_from_buffer(const double* values, std::size_t count) {
    if (values == nullptr && count != 0) {
        throw std::invalid_argument("probability buffer is null");
    }
    return std::vector<double>(values, values + count);
}

}  // namespace

struct CoolBoxBanditArmModel {
    BanditArm impl;

    explicit CoolBoxBanditArmModel(double true_reward_prob)
        : impl(true_reward_prob) {}
};

struct CoolBoxBanditAgentModel {
    std::unique_ptr<BanditAgent> impl;
    std::size_t arm_count;

    CoolBoxBanditAgentModel(std::unique_ptr<BanditAgent> agent_impl, std::size_t count)
        : impl(std::move(agent_impl)), arm_count(count) {}
};

extern "C" {

CoolBoxBanditArmModel* coolbox_create_bandit_arm(double true_reward_prob, char** error_message) {
    clear_error(error_message);
    try {
        return new CoolBoxBanditArmModel(true_reward_prob);
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return nullptr;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return nullptr;
    }
}

int coolbox_bandit_arm_pull(CoolBoxBanditArmModel* model, double* out_reward, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("bandit arm model is null");
        }
        if (out_reward == nullptr) {
            throw std::invalid_argument("bandit arm reward output is null");
        }
        *out_reward = model->impl.pull();
    });
}

int coolbox_bandit_arm_update(CoolBoxBanditArmModel* model, double reward, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("bandit arm model is null");
        }
        model->impl.update(reward);
    });
}

double coolbox_bandit_arm_estimated_prob(const CoolBoxBanditArmModel* model) {
    if (model == nullptr) {
        return 0.0;
    }
    return model->impl.get_estimated_prob();
}

int coolbox_bandit_arm_pull_count(const CoolBoxBanditArmModel* model) {
    if (model == nullptr) {
        return 0;
    }
    return model->impl.get_pull_count();
}

double coolbox_bandit_arm_true_prob(const CoolBoxBanditArmModel* model) {
    if (model == nullptr) {
        return 0.0;
    }
    return model->impl.get_true_prob();
}

void coolbox_free_bandit_arm(CoolBoxBanditArmModel* model) {
    delete model;
}

CoolBoxBanditAgentModel* coolbox_create_epsilon_greedy_agent(const double* true_probs, size_t count, double epsilon, double unused, long long seed, char** error_message) {
    clear_error(error_message);
    try {
        (void)unused;
        std::vector<double> probabilities = probabilities_from_buffer(true_probs, count);
        return new CoolBoxBanditAgentModel(std::make_unique<EpsilonGreedyAgent>(probabilities, epsilon, seed), count);
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return nullptr;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return nullptr;
    }
}

CoolBoxBanditAgentModel* coolbox_create_ucb_agent(const double* true_probs, size_t count, double c, double unused, long long seed, char** error_message) {
    clear_error(error_message);
    try {
        (void)unused;
        (void)seed;
        std::vector<double> probabilities = probabilities_from_buffer(true_probs, count);
        return new CoolBoxBanditAgentModel(std::make_unique<UCBAgent>(probabilities, c), count);
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return nullptr;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return nullptr;
    }
}

CoolBoxBanditAgentModel* coolbox_create_thompson_sampling_agent(const double* true_probs, size_t count, double unused1, double unused2, long long seed, char** error_message) {
    clear_error(error_message);
    try {
        (void)unused1;
        (void)unused2;
        std::vector<double> probabilities = probabilities_from_buffer(true_probs, count);
        return new CoolBoxBanditAgentModel(std::make_unique<ThompsonSamplingAgent>(probabilities, seed), count);
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return nullptr;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return nullptr;
    }
}

CoolBoxBanditAgentModel* coolbox_create_decaying_epsilon_greedy_agent(const double* true_probs, size_t count, double initial_epsilon, double decay_rate, long long seed, char** error_message) {
    clear_error(error_message);
    try {
        std::vector<double> probabilities = probabilities_from_buffer(true_probs, count);
        return new CoolBoxBanditAgentModel(std::make_unique<DecayingEpsilonGreedyAgent>(probabilities, initial_epsilon, decay_rate, seed), count);
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return nullptr;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return nullptr;
    }
}

int coolbox_bandit_agent_run_simulation(CoolBoxBanditAgentModel* model, int num_steps, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr || model->impl == nullptr) {
            throw std::invalid_argument("bandit agent model is null");
        }
        model->impl->run_simulation(num_steps);
    });
}

size_t coolbox_bandit_agent_arm_count(const CoolBoxBanditAgentModel* model) {
    if (model == nullptr) {
        return 0;
    }
    return model->arm_count;
}

int coolbox_bandit_agent_get_results(const CoolBoxBanditAgentModel* model, double* out_true_probs, double* out_estimated_probs, int* out_pull_counts, size_t count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr || model->impl == nullptr) {
            throw std::invalid_argument("bandit agent model is null");
        }
        if (count != model->arm_count) {
            throw std::invalid_argument("bandit result buffer size does not match arm count");
        }
        if ((out_true_probs == nullptr || out_estimated_probs == nullptr || out_pull_counts == nullptr) && count != 0) {
            throw std::invalid_argument("bandit result output buffer is null");
        }

        const SimulationResult result = model->impl->get_results();
        if (result.bandit_results.size() != count) {
            throw std::runtime_error("bandit result count does not match arm count");
        }
        for (std::size_t index = 0; index < count; ++index) {
            out_true_probs[index] = result.bandit_results[index].true_probability;
            out_estimated_probs[index] = result.bandit_results[index].estimated_probability;
            out_pull_counts[index] = result.bandit_results[index].times_pulled;
        }
    });
}

void coolbox_free_bandit_agent(CoolBoxBanditAgentModel* model) {
    delete model;
}

}  // extern "C"