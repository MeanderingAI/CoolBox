#include "../abi/bayesian_network.h"

#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../packages/ML/bayesian_network_ai/headers/bayesian_network.h"

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

std::vector<std::string> states_from_c_strings(const char* const* states, std::size_t state_count) {
    if (states == nullptr && state_count != 0) {
        throw std::invalid_argument("state label buffer is null");
    }

    std::vector<std::string> result;
    result.reserve(state_count);
    for (std::size_t index = 0; index < state_count; ++index) {
        if (states[index] == nullptr) {
            throw std::invalid_argument("state label is null");
        }
        result.emplace_back(states[index]);
    }
    return result;
}

std::vector<double> values_from_buffer(const double* values, std::size_t value_count) {
    if (values == nullptr && value_count != 0) {
        throw std::invalid_argument("value buffer is null");
    }
    return std::vector<double>(values, values + value_count);
}

std::map<int, int> evidence_from_buffers(
    const int* evidence_node_ids,
    const int* evidence_state_ids,
    std::size_t evidence_count) {
    if ((evidence_node_ids == nullptr || evidence_state_ids == nullptr) && evidence_count != 0) {
        throw std::invalid_argument("evidence buffers are null");
    }

    std::map<int, int> evidence;
    for (std::size_t index = 0; index < evidence_count; ++index) {
        evidence[evidence_node_ids[index]] = evidence_state_ids[index];
    }
    return evidence;
}

}  // namespace

struct CoolBoxBayesianNetworkModel {
    BayesianNetwork impl;
};

extern "C" {

CoolBoxBayesianNetworkModel* coolbox_create_bayesian_network(void) {
    return new CoolBoxBayesianNetworkModel();
}

int coolbox_bayesian_network_add_node(CoolBoxBayesianNetworkModel* model, const char* name, const char* const* states, size_t state_count, int* out_node_id, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("bayesian network model is null");
        }
        if (name == nullptr) {
            throw std::invalid_argument("node name is null");
        }
        if (out_node_id == nullptr) {
            throw std::invalid_argument("output node id buffer is null");
        }
        *out_node_id = model->impl.add_node(name, states_from_c_strings(states, state_count));
    });
}

int coolbox_bayesian_network_add_edge(CoolBoxBayesianNetworkModel* model, int parent_id, int child_id, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("bayesian network model is null");
        }
        model->impl.add_edge(parent_id, child_id);
    });
}

int coolbox_bayesian_network_set_cpt(CoolBoxBayesianNetworkModel* model, int node_id, const double* values, size_t value_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("bayesian network model is null");
        }
        model->impl.set_cpt(node_id, values_from_buffer(values, value_count));
    });
}

size_t coolbox_bayesian_network_num_nodes(const CoolBoxBayesianNetworkModel* model) {
    if (model == nullptr) {
        return 0;
    }
    return static_cast<size_t>(model->impl.num_nodes());
}

int coolbox_bayesian_network_get_node_id(const CoolBoxBayesianNetworkModel* model, const char* name, int* out_node_id, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("bayesian network model is null");
        }
        if (name == nullptr) {
            throw std::invalid_argument("node name is null");
        }
        if (out_node_id == nullptr) {
            throw std::invalid_argument("output node id buffer is null");
        }
        *out_node_id = model->impl.get_node_id(name);
    });
}

int coolbox_bayesian_network_get_node_state_count(const CoolBoxBayesianNetworkModel* model, int node_id, size_t* out_state_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("bayesian network model is null");
        }
        if (out_state_count == nullptr) {
            throw std::invalid_argument("output state count buffer is null");
        }
        *out_state_count = model->impl.node(node_id).states.size();
    });
}

int coolbox_bayesian_network_query(const CoolBoxBayesianNetworkModel* model, int query_node, const int* evidence_node_ids, const int* evidence_state_ids, size_t evidence_count, double* out_distribution, size_t distribution_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("bayesian network model is null");
        }
        if (out_distribution == nullptr && distribution_count != 0) {
            throw std::invalid_argument("query output buffer is null");
        }

        const std::vector<double> distribution = model->impl.query(
            query_node,
            evidence_from_buffers(evidence_node_ids, evidence_state_ids, evidence_count));
        if (distribution.size() != distribution_count) {
            throw std::invalid_argument("query output size does not match node state count");
        }
        for (std::size_t index = 0; index < distribution.size(); ++index) {
            out_distribution[index] = distribution[index];
        }
    });
}

void coolbox_free_bayesian_network(CoolBoxBayesianNetworkModel* model) {
    delete model;
}

}  // extern "C"