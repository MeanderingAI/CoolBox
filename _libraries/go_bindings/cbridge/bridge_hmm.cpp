#include "../abi/hmm.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../packages/ML/hidden_markov_model/headers/hidden_markov_model.h"

#include "../../packages/ML/hidden_markov_model/source/hidden_markov_model.cpp"

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

Eigen::VectorXd vector_from_buffer(const double* values, std::size_t count) {
    if (values == nullptr && count != 0) {
        throw std::invalid_argument("input vector buffer is null");
    }

    Eigen::VectorXd vector(static_cast<Eigen::Index>(count));
    for (std::size_t index = 0; index < count; ++index) {
        vector(static_cast<Eigen::Index>(index)) = values[index];
    }
    return vector;
}

Eigen::MatrixXd matrix_from_row_major(const double* flat_values, std::size_t rows, std::size_t cols) {
    if (flat_values == nullptr && rows * cols != 0) {
        throw std::invalid_argument("input matrix buffer is null");
    }

    Eigen::MatrixXd matrix(static_cast<Eigen::Index>(rows), static_cast<Eigen::Index>(cols));
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t col = 0; col < cols; ++col) {
            matrix(static_cast<Eigen::Index>(row), static_cast<Eigen::Index>(col)) = flat_values[row * cols + col];
        }
    }
    return matrix;
}

void copy_vector_to_buffer(const Eigen::VectorXd& vector, double* out_values, std::size_t count) {
    const std::size_t required = static_cast<std::size_t>(vector.size());
    if (count != required) {
        throw std::invalid_argument("output vector size does not match model size");
    }
    if (required != 0 && out_values == nullptr) {
        throw std::invalid_argument("output vector buffer is null");
    }

    for (Eigen::Index index = 0; index < vector.size(); ++index) {
        out_values[static_cast<std::size_t>(index)] = vector(index);
    }
}

void copy_matrix_to_row_major(const Eigen::MatrixXd& matrix, double* out_values, std::size_t rows, std::size_t cols) {
    if (rows != static_cast<std::size_t>(matrix.rows()) || cols != static_cast<std::size_t>(matrix.cols())) {
        throw std::invalid_argument("output matrix shape does not match model shape");
    }
    if (rows * cols != 0 && out_values == nullptr) {
        throw std::invalid_argument("output matrix buffer is null");
    }

    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t col = 0; col < cols; ++col) {
            out_values[row * cols + col] = matrix(static_cast<Eigen::Index>(row), static_cast<Eigen::Index>(col));
        }
    }
}

std::vector<int> observations_from_buffer(const int* observations, std::size_t count) {
    if (observations == nullptr && count != 0) {
        throw std::invalid_argument("observation buffer is null");
    }
    return std::vector<int>(observations, observations + count);
}

std::vector<std::vector<int>> sequences_from_flat_buffer(
    const int* sequences_flat,
    const std::size_t* sequence_lengths,
    std::size_t sequence_count) {
    if (sequence_lengths == nullptr && sequence_count != 0) {
        throw std::invalid_argument("sequence length buffer is null");
    }

    std::size_t total_values = 0;
    for (std::size_t index = 0; index < sequence_count; ++index) {
        total_values += sequence_lengths[index];
    }
    if (sequences_flat == nullptr && total_values != 0) {
        throw std::invalid_argument("sequence value buffer is null");
    }

    std::vector<std::vector<int>> sequences;
    sequences.reserve(sequence_count);
    std::size_t offset = 0;
    for (std::size_t index = 0; index < sequence_count; ++index) {
        const std::size_t length = sequence_lengths[index];
        sequences.emplace_back(sequences_flat + offset, sequences_flat + offset + length);
        offset += length;
    }
    return sequences;
}

}  // namespace

struct CoolBoxHMMModel {
    HMM impl;
    std::size_t state_count;
    std::size_t observation_count;

    CoolBoxHMMModel(std::size_t states, std::size_t observations)
        : impl(static_cast<int>(states), static_cast<int>(observations)),
          state_count(states),
          observation_count(observations) {}
};

extern "C" {

CoolBoxHMMModel* coolbox_create_hmm(size_t states, size_t observations, char** error_message) {
    clear_error(error_message);
    try {
        if (states == 0 || observations == 0) {
            throw std::invalid_argument("hmm states and observations must be positive");
        }
        return new CoolBoxHMMModel(states, observations);
    } catch (const std::exception& ex) {
        set_error(error_message, ex.what());
        return nullptr;
    } catch (...) {
        set_error(error_message, "unexpected bridge error");
        return nullptr;
    }
}

int coolbox_hmm_set_initial_probabilities(CoolBoxHMMModel* model, const double* values, size_t count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        if (count != model->state_count) {
            throw std::invalid_argument("initial probability count must match the number of states");
        }
        model->impl.set_initial_probabilities(vector_from_buffer(values, count));
    });
}

int coolbox_hmm_set_transition_matrix(CoolBoxHMMModel* model, const double* values, size_t rows, size_t cols, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        if (rows != model->state_count || cols != model->state_count) {
            throw std::invalid_argument("transition matrix shape must be states x states");
        }
        model->impl.set_transition_matrix(matrix_from_row_major(values, rows, cols));
    });
}

int coolbox_hmm_set_emission_matrix(CoolBoxHMMModel* model, const double* values, size_t rows, size_t cols, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        if (rows != model->state_count || cols != model->observation_count) {
            throw std::invalid_argument("emission matrix shape must be states x observations");
        }
        model->impl.set_emission_matrix(matrix_from_row_major(values, rows, cols));
    });
}

int coolbox_hmm_get_initial_probabilities(const CoolBoxHMMModel* model, double* out_values, size_t count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        copy_vector_to_buffer(model->impl.get_initial_probabilities(), out_values, count);
    });
}

int coolbox_hmm_get_transition_matrix(const CoolBoxHMMModel* model, double* out_values, size_t rows, size_t cols, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        copy_matrix_to_row_major(model->impl.get_transition_matrix(), out_values, rows, cols);
    });
}

int coolbox_hmm_get_emission_matrix(const CoolBoxHMMModel* model, double* out_values, size_t rows, size_t cols, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        copy_matrix_to_row_major(model->impl.get_emission_matrix(), out_values, rows, cols);
    });
}

int coolbox_hmm_log_likelihood(const CoolBoxHMMModel* model, const int* observations, size_t observation_count, double* out_value, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        if (out_value == nullptr) {
            throw std::invalid_argument("log likelihood output buffer is null");
        }
        *out_value = model->impl.log_likelihood(observations_from_buffer(observations, observation_count));
    });
}

int coolbox_hmm_get_most_likely_states(const CoolBoxHMMModel* model, const int* observations, size_t observation_count, int* out_states, size_t state_count, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        if (state_count != observation_count) {
            throw std::invalid_argument("decoded state buffer length must match observation count");
        }
        if (state_count != 0 && out_states == nullptr) {
            throw std::invalid_argument("decoded state buffer is null");
        }

        const std::vector<int> decoded = model->impl.get_most_likely_states(observations_from_buffer(observations, observation_count));
        for (std::size_t index = 0; index < decoded.size(); ++index) {
            out_states[index] = decoded[index];
        }
    });
}

int coolbox_hmm_train(CoolBoxHMMModel* model, const int* sequences_flat, const size_t* sequence_lengths, size_t sequence_count, int max_iterations, double tolerance, double smoothing_factor, uint32_t seed, char** error_message) {
    return run_bridge_call(error_message, [&]() {
        if (model == nullptr) {
            throw std::invalid_argument("hmm model is null");
        }
        model->impl.train(
            sequences_from_flat_buffer(sequences_flat, sequence_lengths, sequence_count),
            max_iterations,
            tolerance,
            smoothing_factor,
            seed);
    });
}

void coolbox_free_hmm(CoolBoxHMMModel* model) {
    delete model;
}

}  // extern "C"