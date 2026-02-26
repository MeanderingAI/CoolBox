#include "layer.h"
#include <random>
#include <cmath>

namespace ml {
namespace deep_learning {

DenseLayer::DenseLayer(size_t input_size, size_t output_size)
    : input_size_(input_size), output_size_(output_size) {
    weights_ = Tensor({input_size, output_size});
    bias_ = Tensor({output_size}, 0.0);
    weight_gradient_ = Tensor({input_size, output_size}, 0.0);
    bias_gradient_ = Tensor({output_size}, 0.0);

    // Xavier initialization
    double limit = std::sqrt(6.0 / (input_size + output_size));
    weights_.randomize(-limit, limit);
}

Tensor DenseLayer::forward(const Tensor& input) {
    last_input_ = input;
    // output = input * weights + bias
    Tensor output = input.matmul(weights_);
    // Add bias (broadcast)
    for (size_t i = 0; i < output.shape()[0]; ++i) {
        for (size_t j = 0; j < output_size_; ++j) {
            output.data()[i * output_size_ + j] += bias_.data()[j];
        }
    }
    last_output_ = output;
    return output;
}

Tensor DenseLayer::backward(const Tensor& gradient) {
    // weight_gradient = input^T * gradient
    weight_gradient_ = last_input_.transpose().matmul(gradient);
    
    // bias_gradient = sum of gradient along batch dimension
    bias_gradient_ = Tensor({output_size_}, 0.0);
    for (size_t i = 0; i < gradient.shape()[0]; ++i) {
        for (size_t j = 0; j < output_size_; ++j) {
            bias_gradient_.data()[j] += gradient.data()[i * output_size_ + j];
        }
    }

    // Return gradient w.r.t. input = gradient * weights^T
    return gradient.matmul(weights_.transpose());
}

void DenseLayer::update_parameters(double learning_rate) {
    // weights -= lr * weight_gradient
    double batch_size = static_cast<double>(last_input_.shape()[0]);
    for (size_t i = 0; i < weights_.size(); ++i) {
        weights_.data()[i] -= learning_rate * weight_gradient_.data()[i] / batch_size;
    }
    for (size_t i = 0; i < bias_.size(); ++i) {
        bias_.data()[i] -= learning_rate * bias_gradient_.data()[i] / batch_size;
    }
}

} // namespace deep_learning
} // namespace ml
