#include "dl_bridge.h"

#include <DL/layers/include/layer.h>
#include <DL/layers/include/tensor.h>
#include <DL/loss/include/loss.h>
#include <DL/optimizer/include/optimizer.h>
#include <DL/wrapper/include/neural_network.h>
#include <llf_helix.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using ml::deep_learning::Layer;
using ml::deep_learning::Tensor;

void write_error(char* output, size_t capacity, const char* message) {
    if (!output || capacity == 0) return;
    const size_t count = std::min(capacity - 1, std::strlen(message));
    std::memcpy(output, message, count);
    output[count] = '\0';
}

template <typename Function>
int invoke(char* error, size_t capacity, Function&& function) {
    try {
        function();
        return 1;
    } catch (const std::exception& exception) {
        write_error(error, capacity, exception.what());
    } catch (...) {
        write_error(error, capacity, "unknown native exception");
    }
    return 0;
}

template <typename Result, typename Function>
Result* invoke_pointer(char* error, size_t capacity, Function&& function) {
    try {
        return new Result(function());
    } catch (const std::exception& exception) {
        write_error(error, capacity, exception.what());
    } catch (...) {
        write_error(error, capacity, "unknown native exception");
    }
    return nullptr;
}

Tensor& tensor(void* handle) {
    if (!handle) throw std::invalid_argument("tensor is closed");
    return *static_cast<Tensor*>(handle);
}

std::shared_ptr<Layer>& layer(void* handle) {
    if (!handle) throw std::invalid_argument("layer is closed");
    return *static_cast<std::shared_ptr<Layer>*>(handle);
}

std::shared_ptr<ml::deep_learning::Loss>& loss(void* handle) {
    if (!handle) throw std::invalid_argument("loss is closed");
    return *static_cast<std::shared_ptr<ml::deep_learning::Loss>*>(handle);
}

std::shared_ptr<ml::deep_learning::Optimizer>& optimizer(void* handle) {
    if (!handle) throw std::invalid_argument("optimizer is closed");
    return *static_cast<std::shared_ptr<ml::deep_learning::Optimizer>*>(handle);
}

ml::deep_learning::NeuralNetwork& network(void* handle) {
    if (!handle) throw std::invalid_argument("neural network is closed");
    return *static_cast<ml::deep_learning::NeuralNetwork*>(handle);
}

ml::deep_learning::agents::Helix& helix(void* handle) {
    if (!handle) throw std::invalid_argument("HELiX learner is closed");
    return *static_cast<ml::deep_learning::agents::Helix*>(handle);
}

size_t parameter(const size_t* values, size_t count, size_t index) {
    if (!values || index >= count) {
        throw std::invalid_argument("missing layer constructor parameter");
    }
    return values[index];
}

void copy_text(const std::string& value, char* output, size_t capacity) {
    if (!output || capacity == 0) return;
    const size_t count = std::min(capacity - 1, value.size());
    std::memcpy(output, value.data(), count);
    output[count] = '\0';
}

} // namespace

extern "C" {

void* coolbox_dl_tensor_new(const size_t* shape, size_t dimensions,
                            const double* values, size_t value_count,
                            char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] {
        if (!shape || dimensions == 0) {
            throw std::invalid_argument("tensor shape must have at least one dimension");
        }
        std::vector<size_t> tensor_shape(shape, shape + dimensions);
        size_t expected = 1;
        for (size_t dimension : tensor_shape) {
            if (dimension == 0 || expected > std::numeric_limits<size_t>::max() / dimension) {
                throw std::invalid_argument("tensor dimensions must be positive and not overflow");
            }
            expected *= dimension;
        }
        if (value_count != 0 && (value_count != expected || !values)) {
            throw std::invalid_argument("tensor data length does not match shape");
        }
        if (value_count == 0) return Tensor(tensor_shape);
        return Tensor(tensor_shape, std::vector<double>(values, values + value_count));
    });
}

void* coolbox_dl_tensor_clone(void* handle, char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] { return tensor(handle).clone(); });
}

void* coolbox_dl_tensor_reshape(void* handle, const size_t* shape, size_t dimensions,
                                char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] {
        if (!shape || dimensions == 0) throw std::invalid_argument("reshape shape is empty");
        return tensor(handle).reshape(std::vector<size_t>(shape, shape + dimensions));
    });
}

void* coolbox_dl_tensor_transpose(void* handle, char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] { return tensor(handle).transpose(); });
}

void* coolbox_dl_tensor_binary(void* lhs, void* rhs, int operation,
                               char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] {
        switch (operation) {
        case 0: return tensor(lhs) + tensor(rhs);
        case 1: return tensor(lhs) - tensor(rhs);
        case 2: return tensor(lhs) * tensor(rhs);
        case 3: return tensor(lhs) / tensor(rhs);
        default: throw std::invalid_argument("unknown tensor binary operation");
        }
    });
}

void* coolbox_dl_tensor_scalar(void* handle, double scalar, int divide,
                               char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] {
        return divide ? tensor(handle) / scalar : tensor(handle) * scalar;
    });
}

void* coolbox_dl_tensor_matmul(void* lhs, void* rhs, char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] { return tensor(lhs).matmul(tensor(rhs)); });
}

void coolbox_dl_tensor_free(void* handle) { delete static_cast<Tensor*>(handle); }
size_t coolbox_dl_tensor_dimensions(void* handle) { return handle ? tensor(handle).ndim() : 0; }
size_t coolbox_dl_tensor_size(void* handle) { return handle ? tensor(handle).size() : 0; }

int coolbox_dl_tensor_shape(void* handle, size_t* shape, size_t capacity,
                            char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        const auto& dimensions = tensor(handle).shape();
        if (!shape || capacity < dimensions.size()) throw std::invalid_argument("tensor shape buffer is too small");
        std::copy(dimensions.begin(), dimensions.end(), shape);
    });
}

int coolbox_dl_tensor_data(void* handle, double* values, size_t capacity,
                           char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        const auto& data = tensor(handle).data();
        if (!values || capacity < data.size()) throw std::invalid_argument("tensor data buffer is too small");
        std::copy(data.begin(), data.end(), values);
    });
}

int coolbox_dl_tensor_fill(void* handle, double value, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { tensor(handle).fill(value); });
}

int coolbox_dl_tensor_randomize(void* handle, double minimum, double maximum,
                                char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!(minimum < maximum)) throw std::invalid_argument("randomize minimum must be less than maximum");
        tensor(handle).randomize(minimum, maximum);
    });
}

int coolbox_dl_tensor_at(void* handle, size_t index, double* value,
                         char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!value) throw std::invalid_argument("tensor value output is null");
        *value = tensor(handle).at(index);
    });
}

int coolbox_dl_tensor_set(void* handle, size_t index, double value,
                          char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { tensor(handle).at(index) = value; });
}

void* coolbox_dl_layer_new(int kind, const size_t* parameters, size_t count,
                           double rate, int return_sequences,
                           char* error, size_t error_capacity) {
    using namespace ml::deep_learning;
    return invoke_pointer<std::shared_ptr<Layer>>(error, error_capacity, [&] {
        switch (kind) {
        case 0: return std::static_pointer_cast<Layer>(std::make_shared<DenseLayer>(parameter(parameters, count, 0), parameter(parameters, count, 1)));
        case 1: return std::static_pointer_cast<Layer>(std::make_shared<ReLULayer>());
        case 2: return std::static_pointer_cast<Layer>(std::make_shared<SigmoidLayer>());
        case 3: return std::static_pointer_cast<Layer>(std::make_shared<TanhLayer>());
        case 4: return std::static_pointer_cast<Layer>(std::make_shared<SoftmaxLayer>());
        case 5: return std::static_pointer_cast<Layer>(std::make_shared<DropoutLayer>(rate));
        case 6: return std::static_pointer_cast<Layer>(std::make_shared<BatchNormLayer>(parameter(parameters, count, 0)));
        case 7: return std::static_pointer_cast<Layer>(std::make_shared<LayerNormLayer>(parameter(parameters, count, 0)));
        case 8: return std::static_pointer_cast<Layer>(std::make_shared<FlattenLayer>());
        case 9: return std::static_pointer_cast<Layer>(std::make_shared<Conv1DLayer>(parameter(parameters, count, 0), parameter(parameters, count, 1), parameter(parameters, count, 2)));
        case 10: return std::static_pointer_cast<Layer>(std::make_shared<Conv2DLayer>(parameter(parameters, count, 0), parameter(parameters, count, 1), parameter(parameters, count, 2), parameter(parameters, count, 3), parameter(parameters, count, 4), parameter(parameters, count, 5)));
        case 11: return std::static_pointer_cast<Layer>(std::make_shared<MaxPool2DLayer>(parameter(parameters, count, 0), parameter(parameters, count, 1), parameter(parameters, count, 2), parameter(parameters, count, 3)));
        case 12: return std::static_pointer_cast<Layer>(std::make_shared<AvgPool2DLayer>(parameter(parameters, count, 0), parameter(parameters, count, 1), parameter(parameters, count, 2), parameter(parameters, count, 3)));
        case 13: return std::static_pointer_cast<Layer>(std::make_shared<RNNLayer>(parameter(parameters, count, 0), parameter(parameters, count, 1), return_sequences != 0));
        case 14: return std::static_pointer_cast<Layer>(std::make_shared<LSTMLayer>(parameter(parameters, count, 0), parameter(parameters, count, 1), return_sequences != 0));
        case 15: return std::static_pointer_cast<Layer>(std::make_shared<GRULayer>(parameter(parameters, count, 0), parameter(parameters, count, 1), return_sequences != 0));
        case 16: return std::static_pointer_cast<Layer>(std::make_shared<EmbeddingLayer>(parameter(parameters, count, 0), parameter(parameters, count, 1)));
        default: throw std::invalid_argument("unknown deep-learning layer kind");
        }
    });
}

void coolbox_dl_layer_free(void* handle) { delete static_cast<std::shared_ptr<Layer>*>(handle); }

void* coolbox_dl_layer_forward(void* handle, void* input, char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] { return layer(handle)->forward(tensor(input)); });
}

void* coolbox_dl_layer_backward(void* handle, void* gradient, char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] { return layer(handle)->backward(tensor(gradient)); });
}

int coolbox_dl_layer_update(void* handle, double learning_rate, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!(learning_rate > 0.0)) throw std::invalid_argument("learning rate must be positive");
        layer(handle)->update_parameters(learning_rate);
    });
}

int coolbox_dl_layer_set_training(void* handle, int training, char* error, size_t error_capacity) {
    using namespace ml::deep_learning;
    return invoke(error, error_capacity, [&] {
        auto& target = layer(handle);
        if (auto* dropout = dynamic_cast<DropoutLayer*>(target.get())) dropout->set_training(training != 0);
        else if (auto* batch_norm = dynamic_cast<BatchNormLayer*>(target.get())) batch_norm->set_training(training != 0);
        else throw std::invalid_argument("layer does not support training-mode changes");
    });
}

int coolbox_dl_layer_has_parameters(void* handle) {
    return handle && layer(handle)->has_parameters();
}

int coolbox_dl_layer_name(void* handle, char* output, size_t capacity,
                          char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!output || capacity == 0) throw std::invalid_argument("layer name output buffer is empty");
        copy_text(layer(handle)->name(), output, capacity);
    });
}

void* coolbox_dl_loss_new(int kind, char* error, size_t error_capacity) {
    using namespace ml::deep_learning;
    return invoke_pointer<std::shared_ptr<Loss>>(error, error_capacity, [&] {
        switch (kind) {
        case 0: return std::static_pointer_cast<Loss>(std::make_shared<MSELoss>());
        case 1: return std::static_pointer_cast<Loss>(std::make_shared<BCELoss>());
        case 2: return std::static_pointer_cast<Loss>(std::make_shared<CategoricalCrossEntropyLoss>());
        default: throw std::invalid_argument("unknown loss kind");
        }
    });
}

void coolbox_dl_loss_free(void* handle) { delete static_cast<std::shared_ptr<ml::deep_learning::Loss>*>(handle); }

int coolbox_dl_loss_compute(void* handle, void* predictions, void* targets,
                            double* value, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!value) throw std::invalid_argument("loss output is null");
        *value = loss(handle)->compute(tensor(predictions), tensor(targets));
    });
}

void* coolbox_dl_loss_gradient(void* handle, void* predictions, void* targets,
                               char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] {
        return loss(handle)->gradient(tensor(predictions), tensor(targets));
    });
}

void* coolbox_dl_optimizer_new(int kind, double learning_rate, double parameter1,
                               double parameter2, char* error, size_t error_capacity) {
    using namespace ml::deep_learning;
    return invoke_pointer<std::shared_ptr<Optimizer>>(error, error_capacity, [&] {
        switch (kind) {
        case 0: return std::static_pointer_cast<Optimizer>(std::make_shared<SGD>(learning_rate, parameter1));
        case 1: return std::static_pointer_cast<Optimizer>(std::make_shared<Adam>(learning_rate, parameter1, parameter2));
        case 2: return std::static_pointer_cast<Optimizer>(std::make_shared<RMSprop>(learning_rate, parameter1));
        default: throw std::invalid_argument("unknown optimizer kind");
        }
    });
}

void coolbox_dl_optimizer_free(void* handle) { delete static_cast<std::shared_ptr<ml::deep_learning::Optimizer>*>(handle); }

int coolbox_dl_optimizer_step(void* handle, void* parameters, void* gradients,
                              char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        optimizer(handle)->step(tensor(parameters), tensor(gradients));
    });
}

int coolbox_dl_optimizer_reset(void* handle, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { optimizer(handle)->reset(); });
}

int coolbox_dl_optimizer_name(void* handle, char* output, size_t capacity,
                              char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!output || capacity == 0) throw std::invalid_argument("optimizer name output buffer is empty");
        copy_text(optimizer(handle)->name(), output, capacity);
    });
}

void* coolbox_dl_network_new(char* error, size_t error_capacity) {
    return invoke_pointer<ml::deep_learning::NeuralNetwork>(error, error_capacity, [] {
        return ml::deep_learning::NeuralNetwork();
    });
}

void coolbox_dl_network_free(void* handle) { delete static_cast<ml::deep_learning::NeuralNetwork*>(handle); }

int coolbox_dl_network_add_layer(void* handle, void* layer_handle, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { network(handle).add_layer(layer(layer_handle)); });
}

int coolbox_dl_network_set_loss(void* handle, void* loss_handle, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { network(handle).set_loss(loss(loss_handle)); });
}

int coolbox_dl_network_set_optimizer(void* handle, void* optimizer_handle,
                                     char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { network(handle).set_optimizer(optimizer(optimizer_handle)); });
}

void* coolbox_dl_network_predict(void* handle, void* input, char* error, size_t error_capacity) {
    return invoke_pointer<Tensor>(error, error_capacity, [&] { return network(handle).predict(tensor(input)); });
}

int coolbox_dl_network_train_step(void* handle, void* input, void* target,
                                  double* value, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!value) throw std::invalid_argument("train loss output is null");
        *value = network(handle).train_step(tensor(input), tensor(target));
    });
}

int coolbox_dl_network_evaluate(void* handle, void** inputs, void** targets,
                                size_t count, double* value,
                                char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!value || !inputs || !targets || count == 0) {
            throw std::invalid_argument("evaluation requires non-empty inputs and targets");
        }
        std::vector<Tensor> input_values, target_values;
        input_values.reserve(count);
        target_values.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            input_values.push_back(tensor(inputs[i]));
            target_values.push_back(tensor(targets[i]));
        }
        *value = network(handle).evaluate(input_values, target_values);
    });
}

int coolbox_dl_network_train(void* handle, void** inputs, void** targets,
                             size_t count, int epochs, int batch_size, int verbose,
                             char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!inputs || !targets || count == 0 || epochs < 0 || batch_size <= 0) {
            throw std::invalid_argument("training requires matching non-empty tensors, non-negative epochs, and a positive batch size");
        }
        std::vector<Tensor> input_values, target_values;
        input_values.reserve(count);
        target_values.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            input_values.push_back(tensor(inputs[i]));
            target_values.push_back(tensor(targets[i]));
        }
        network(handle).train(input_values, target_values, epochs, batch_size, verbose != 0);
    });
}

int coolbox_dl_network_set_training(void* handle, int training, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { network(handle).set_training(training != 0); });
}

size_t coolbox_dl_network_layer_count(void* handle) {
    return handle ? network(handle).num_layers() : 0;
}

void* coolbox_dl_helix_new(size_t actions, size_t truth, int rich_feedback,
                           char* error, size_t error_capacity) {
    return invoke_pointer<ml::deep_learning::agents::Helix>(error, error_capacity, [&] {
        auto problem = rich_feedback
            ? ml::deep_learning::agents::make_rich_feedback_problem(actions, truth)
            : ml::deep_learning::agents::make_reward_only_problem(actions, truth);
        return ml::deep_learning::agents::Helix(std::move(problem));
    });
}

void coolbox_dl_helix_free(void* handle) { delete static_cast<ml::deep_learning::agents::Helix*>(handle); }

int coolbox_dl_helix_select_action(void* handle, size_t* action,
                                   char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!action) throw std::invalid_argument("action output is null");
        *action = helix(handle).select_action();
    });
}

int coolbox_dl_helix_observe(void* handle, size_t action, const char* feedback,
                             char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!feedback) throw std::invalid_argument("feedback is null");
        helix(handle).observe(action, feedback);
    });
}

int coolbox_dl_helix_interact(void* handle, size_t* action,
                              char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!action) throw std::invalid_argument("action output is null");
        *action = helix(handle).interact();
    });
}

size_t coolbox_dl_helix_rounds(void* handle) { return handle ? helix(handle).rounds() : 0; }
size_t coolbox_dl_helix_mistakes(void* handle) { return handle ? helix(handle).mistakes() : 0; }
double coolbox_dl_helix_cumulative_regret(void* handle) { return handle ? helix(handle).cumulative_regret() : 0.0; }
int coolbox_dl_helix_identified(void* handle) { return handle && helix(handle).identified(); }

size_t coolbox_dl_helix_version_space(void* handle, size_t* indices, size_t capacity) {
    if (!handle) return 0;
    const auto& version_space = helix(handle).version_space();
    if (indices) {
        std::copy_n(version_space.begin(), std::min(capacity, version_space.size()), indices);
    }
    return version_space.size();
}

int coolbox_dl_helix_dimension(size_t actions, size_t truth, int rich_feedback,
                               double epsilon, size_t max_actions, size_t* dimension,
                               char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (!dimension) throw std::invalid_argument("dimension output is null");
        auto problem = rich_feedback
            ? ml::deep_learning::agents::make_rich_feedback_problem(actions, truth)
            : ml::deep_learning::agents::make_reward_only_problem(actions, truth);
        *dimension = ml::deep_learning::agents::transfer_eluder_dimension(problem, epsilon, max_actions);
    });
}

} // extern "C"
