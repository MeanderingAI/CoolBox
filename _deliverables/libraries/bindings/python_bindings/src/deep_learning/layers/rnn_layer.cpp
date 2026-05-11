#include "DL/layers/include/rnn_layer.h"

namespace ml {
namespace deep_learning {

RNNLayer::RNNLayer(int input_size, int hidden_size)
    : input_size_(input_size), hidden_size_(hidden_size) {}

Tensor RNNLayer::forward(const Tensor& input) {
    // Dummy implementation
    return input;
}

Tensor RNNLayer::backward(const Tensor& gradient) {
    // Dummy implementation
    return gradient;
}

void RNNLayer::update_parameters(double learning_rate) {}

} // namespace deep_learning
} // namespace ml
