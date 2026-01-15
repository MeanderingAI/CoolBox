#include "ML/deep_learning/layer.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace ml {
namespace deep_learning {

// Dense Layer Implementation
DenseLayer::DenseLayer(size_t input_size, size_t output_size)
    : input_size_(input_size), output_size_(output_size) {
    
    // Initialize weights with Xavier/Glorot initialization
    weights_ = Tensor({input_size, output_size});
    double limit = std::sqrt(6.0 / (input_size + output_size));
    weights_.randomize(-limit, limit);
    
    // Initialize bias to zeros
    bias_ = Tensor({output_size}, 0.0);
    
    // Initialize gradients
    weight_gradient_ = Tensor({input_size, output_size}, 0.0);
    bias_gradient_ = Tensor({output_size}, 0.0);
}

Tensor DenseLayer::forward(const Tensor& input) {
    last_input_ = input.clone();
    
    if (input.shape().size() != 2) {
        throw std::invalid_argument("DenseLayer expects 2D input [batch_size, features]");
    }
    
    size_t batch_size = input.shape()[0];
    
    // Output = Input @ Weights + Bias
    Tensor output = input.matmul(weights_);
    
    // Add bias to each sample in the batch
    for (size_t i = 0; i < batch_size; ++i) {
        for (size_t j = 0; j < output_size_; ++j) {
            output.data()[i * output_size_ + j] += bias_.data()[j];
        }
    }
    
    last_output_ = output.clone();
    return output;
}

Tensor DenseLayer::backward(const Tensor& gradient) {
    size_t batch_size = gradient.shape()[0];
    
    // Gradient w.r.t. weights: Input^T @ Gradient
    Tensor input_t = last_input_.transpose();
    weight_gradient_ = input_t.matmul(gradient);
    
    // Gradient w.r.t. bias: sum over batch dimension
    bias_gradient_.fill(0.0);
    for (size_t i = 0; i < batch_size; ++i) {
        for (size_t j = 0; j < output_size_; ++j) {
            bias_gradient_.data()[j] += gradient.data()[i * output_size_ + j];
        }
    }
    
    // Gradient w.r.t. input: Gradient @ Weights^T
    Tensor weights_t = weights_.transpose();
    return gradient.matmul(weights_t);
}

void DenseLayer::update_parameters(double learning_rate) {
    // Update weights
    for (size_t i = 0; i < weights_.size(); ++i) {
        weights_.data()[i] -= learning_rate * weight_gradient_.data()[i];
    }
    
    // Update bias
    for (size_t i = 0; i < bias_.size(); ++i) {
        bias_.data()[i] -= learning_rate * bias_gradient_.data()[i];
    }
}

// ReLU Layer Implementation
Tensor ReLULayer::forward(const Tensor& input) {
    last_input_ = input.clone();
    
    Tensor output = input.clone();
    for (auto& val : output.data()) {
        val = std::max(0.0, val);
    }
    
    return output;
}

Tensor ReLULayer::backward(const Tensor& gradient) {
    Tensor result = gradient.clone();
    
    for (size_t i = 0; i < result.size(); ++i) {
        if (last_input_.data()[i] <= 0) {
            result.data()[i] = 0.0;
        }
    }
    
    return result;
}

// Sigmoid Layer Implementation
Tensor SigmoidLayer::forward(const Tensor& input) {
    last_input_ = input.clone();
    
    Tensor output = input.clone();
    for (auto& val : output.data()) {
        val = 1.0 / (1.0 + std::exp(-val));
    }
    
    last_output_ = output.clone();
    return output;
}

Tensor SigmoidLayer::backward(const Tensor& gradient) {
    Tensor result = gradient.clone();
    
    for (size_t i = 0; i < result.size(); ++i) {
        double sigmoid_val = last_output_.data()[i];
        result.data()[i] *= sigmoid_val * (1.0 - sigmoid_val);
    }
    
    return result;
}

// Tanh Layer Implementation
Tensor TanhLayer::forward(const Tensor& input) {
    last_input_ = input.clone();
    
    Tensor output = input.clone();
    for (auto& val : output.data()) {
        val = std::tanh(val);
    }
    
    last_output_ = output.clone();
    return output;
}

Tensor TanhLayer::backward(const Tensor& gradient) {
    Tensor result = gradient.clone();
    
    for (size_t i = 0; i < result.size(); ++i) {
        double tanh_val = last_output_.data()[i];
        result.data()[i] *= (1.0 - tanh_val * tanh_val);
    }
    
    return result;
}

// Softmax Layer Implementation
Tensor SoftmaxLayer::forward(const Tensor& input) {
    last_input_ = input.clone();
    
    if (input.shape().size() != 2) {
        throw std::invalid_argument("SoftmaxLayer expects 2D input [batch_size, features]");
    }
    
    size_t batch_size = input.shape()[0];
    size_t features = input.shape()[1];
    
    Tensor output = input.clone();
    
    // Apply softmax for each sample in the batch
    for (size_t i = 0; i < batch_size; ++i) {
        // Find max for numerical stability
        double max_val = output.data()[i * features];
        for (size_t j = 1; j < features; ++j) {
            max_val = std::max(max_val, output.data()[i * features + j]);
        }
        
        // Compute exp and sum
        double sum = 0.0;
        for (size_t j = 0; j < features; ++j) {
            output.data()[i * features + j] = std::exp(output.data()[i * features + j] - max_val);
            sum += output.data()[i * features + j];
        }
        
        // Normalize
        for (size_t j = 0; j < features; ++j) {
            output.data()[i * features + j] /= sum;
        }
    }
    
    last_output_ = output.clone();
    return output;
}

Tensor SoftmaxLayer::backward(const Tensor& gradient) {
    // For numerical stability and simplicity, when used with cross-entropy loss,
    // the gradient is typically computed together with the loss
    // Here we implement the general case
    
    if (gradient.shape().size() != 2) {
        throw std::invalid_argument("Gradient must be 2D");
    }
    
    size_t batch_size = gradient.shape()[0];
    size_t features = gradient.shape()[1];
    
    Tensor result = gradient.clone();
    
    for (size_t i = 0; i < batch_size; ++i) {
        for (size_t j = 0; j < features; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < features; ++k) {
                double delta = (j == k) ? 1.0 : 0.0;
                sum += gradient.data()[i * features + k] * 
                       last_output_.data()[i * features + j] * 
                       (delta - last_output_.data()[i * features + k]);
            }
            result.data()[i * features + j] = sum;
        }
    }
    
    return result;
}

// Dropout Layer Implementation
DropoutLayer::DropoutLayer(double dropout_rate)
    : dropout_rate_(dropout_rate), training_(true) {
    if (dropout_rate < 0.0 || dropout_rate >= 1.0) {
        throw std::invalid_argument("Dropout rate must be in [0, 1)");
    }
}

Tensor DropoutLayer::forward(const Tensor& input) {
    last_input_ = input.clone();
    
    if (!training_ || dropout_rate_ == 0.0) {
        return input.clone();
    }
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    Tensor output = input.clone();
    mask_ = Tensor(input.shape());
    
    double scale = 1.0 / (1.0 - dropout_rate_);
    
    for (size_t i = 0; i < output.size(); ++i) {
        if (dist(gen) < dropout_rate_) {
            output.data()[i] = 0.0;
            mask_.data()[i] = 0.0;
        } else {
            output.data()[i] *= scale;
            mask_.data()[i] = scale;
        }
    }
    
    return output;
}

Tensor DropoutLayer::backward(const Tensor& gradient) {
    if (!training_ || dropout_rate_ == 0.0) {
        return gradient.clone();
    }
    
    return gradient * mask_;
}

} // namespace deep_learning
} // namespace ml
