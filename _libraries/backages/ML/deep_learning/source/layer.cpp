#include "layer.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cassert>

namespace ml {
namespace deep_learning {

// Helper: apply sigmoid element-wise
static Tensor apply_sigmoid(const Tensor& t) {
    Tensor result = t.clone();
    for (size_t i = 0; i < result.size(); ++i) {
        result.data()[i] = 1.0 / (1.0 + std::exp(-result.data()[i]));
    }
    return result;
}

// Helper: apply tanh element-wise
static Tensor apply_tanh(const Tensor& t) {
    Tensor result = t.clone();
    for (size_t i = 0; i < result.size(); ++i) {
        result.data()[i] = std::tanh(result.data()[i]);
    }
    return result;
}

// Helper: concatenate two 2D tensors along columns [batch, a+b]
static Tensor concat_cols(const Tensor& a, const Tensor& b) {
    size_t batch = a.shape()[0];
    size_t ca = a.shape()[1];
    size_t cb = b.shape()[1];
    Tensor result({batch, ca + cb}, 0.0);
    for (size_t i = 0; i < batch; ++i) {
        for (size_t j = 0; j < ca; ++j)
            result.data()[i * (ca + cb) + j] = a.data()[i * ca + j];
        for (size_t j = 0; j < cb; ++j)
            result.data()[i * (ca + cb) + ca + j] = b.data()[i * cb + j];
    }
    return result;
}

// ============================================================================
// DenseLayer
// ============================================================================

DenseLayer::DenseLayer(size_t input_size, size_t output_size)
    : input_size_(input_size), output_size_(output_size) {
    weights_ = Tensor({input_size, output_size});
    bias_ = Tensor({output_size}, 0.0);
    weight_gradient_ = Tensor({input_size, output_size}, 0.0);
    bias_gradient_ = Tensor({output_size}, 0.0);

    double limit = std::sqrt(6.0 / (input_size + output_size));
    weights_.randomize(-limit, limit);
}

Tensor DenseLayer::forward(const Tensor& input) {
    last_input_ = input;
    Tensor output = input.matmul(weights_);
    for (size_t i = 0; i < output.shape()[0]; ++i) {
        for (size_t j = 0; j < output_size_; ++j) {
            output.data()[i * output_size_ + j] += bias_.data()[j];
        }
    }
    last_output_ = output;
    return output;
}

Tensor DenseLayer::backward(const Tensor& gradient) {
    weight_gradient_ = last_input_.transpose().matmul(gradient);
    
    bias_gradient_ = Tensor({output_size_}, 0.0);
    for (size_t i = 0; i < gradient.shape()[0]; ++i) {
        for (size_t j = 0; j < output_size_; ++j) {
            bias_gradient_.data()[j] += gradient.data()[i * output_size_ + j];
        }
    }

    return gradient.matmul(weights_.transpose());
}

void DenseLayer::update_parameters(double learning_rate) {
    double batch_size = static_cast<double>(last_input_.shape()[0]);
    for (size_t i = 0; i < weights_.size(); ++i) {
        weights_.data()[i] -= learning_rate * weight_gradient_.data()[i] / batch_size;
    }
    for (size_t i = 0; i < bias_.size(); ++i) {
        bias_.data()[i] -= learning_rate * bias_gradient_.data()[i] / batch_size;
    }
}

// ============================================================================
// Activation Layers
// ============================================================================

Tensor ReLULayer::forward(const Tensor& input) {
    last_input_ = input;
    Tensor output = input.clone();
    for (size_t i = 0; i < output.size(); ++i) {
        if (output.data()[i] < 0.0) output.data()[i] = 0.0;
    }
    last_output_ = output;
    return output;
}

Tensor ReLULayer::backward(const Tensor& gradient) {
    Tensor result = gradient.clone();
    for (size_t i = 0; i < result.size(); ++i) {
        if (last_input_.data()[i] <= 0.0) result.data()[i] = 0.0;
    }
    return result;
}

Tensor SigmoidLayer::forward(const Tensor& input) {
    last_input_ = input;
    last_output_ = apply_sigmoid(input);
    return last_output_;
}

Tensor SigmoidLayer::backward(const Tensor& gradient) {
    // sigmoid'(x) = sigmoid(x) * (1 - sigmoid(x))
    Tensor result = gradient.clone();
    for (size_t i = 0; i < result.size(); ++i) {
        double s = last_output_.data()[i];
        result.data()[i] *= s * (1.0 - s);
    }
    return result;
}

Tensor TanhLayer::forward(const Tensor& input) {
    last_input_ = input;
    last_output_ = apply_tanh(input);
    return last_output_;
}

Tensor TanhLayer::backward(const Tensor& gradient) {
    // tanh'(x) = 1 - tanh(x)^2
    Tensor result = gradient.clone();
    for (size_t i = 0; i < result.size(); ++i) {
        double t = last_output_.data()[i];
        result.data()[i] *= (1.0 - t * t);
    }
    return result;
}

Tensor SoftmaxLayer::forward(const Tensor& input) {
    last_input_ = input;
    Tensor output = input.clone();
    // Apply softmax per row (batch dimension)
    size_t batch = input.shape()[0];
    size_t cols = input.size() / batch;
    for (size_t i = 0; i < batch; ++i) {
        double max_val = -std::numeric_limits<double>::infinity();
        for (size_t j = 0; j < cols; ++j)
            max_val = std::max(max_val, output.data()[i * cols + j]);
        double sum = 0.0;
        for (size_t j = 0; j < cols; ++j) {
            output.data()[i * cols + j] = std::exp(output.data()[i * cols + j] - max_val);
            sum += output.data()[i * cols + j];
        }
        for (size_t j = 0; j < cols; ++j)
            output.data()[i * cols + j] /= sum;
    }
    last_output_ = output;
    return output;
}

Tensor SoftmaxLayer::backward(const Tensor& gradient) {
    // Simplified: assumes used with cross-entropy loss where gradient is already (pred - target)
    return gradient;
}

// ============================================================================
// Utility Layers
// ============================================================================

DropoutLayer::DropoutLayer(double rate) : rate_(rate) {}

Tensor DropoutLayer::forward(const Tensor& input) {
    last_input_ = input;
    if (!training_ || rate_ <= 0.0) {
        last_output_ = input;
        return input;
    }
    mask_ = Tensor(input.shape(), 0.0);
    Tensor output = input.clone();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution dist(1.0 - rate_);
    double scale = 1.0 / (1.0 - rate_);
    for (size_t i = 0; i < output.size(); ++i) {
        double keep = dist(gen) ? 1.0 : 0.0;
        mask_.data()[i] = keep;
        output.data()[i] *= keep * scale;
    }
    last_output_ = output;
    return output;
}

Tensor DropoutLayer::backward(const Tensor& gradient) {
    if (!training_ || rate_ <= 0.0) return gradient;
    double scale = 1.0 / (1.0 - rate_);
    Tensor result = gradient.clone();
    for (size_t i = 0; i < result.size(); ++i) {
        result.data()[i] *= mask_.data()[i] * scale;
    }
    return result;
}

// BatchNormLayer
BatchNormLayer::BatchNormLayer(size_t num_features, double epsilon, double momentum)
    : num_features_(num_features), epsilon_(epsilon), momentum_(momentum) {
    gamma_ = Tensor({num_features}, 1.0);
    beta_ = Tensor({num_features}, 0.0);
    running_mean_ = Tensor({num_features}, 0.0);
    running_var_ = Tensor({num_features}, 1.0);
    gamma_gradient_ = Tensor({num_features}, 0.0);
    beta_gradient_ = Tensor({num_features}, 0.0);
}

Tensor BatchNormLayer::forward(const Tensor& input) {
    last_input_ = input;
    size_t batch = input.shape()[0];
    size_t features = num_features_;
    
    Tensor output(input.shape(), 0.0);
    normalized_ = Tensor(input.shape(), 0.0);
    batch_mean_ = Tensor({features}, 0.0);
    batch_var_ = Tensor({features}, 0.0);
    
    if (training_) {
        // Compute mean
        for (size_t i = 0; i < batch; ++i)
            for (size_t j = 0; j < features; ++j)
                batch_mean_.data()[j] += input.data()[i * features + j];
        for (size_t j = 0; j < features; ++j)
            batch_mean_.data()[j] /= batch;
        
        // Compute variance
        for (size_t i = 0; i < batch; ++i)
            for (size_t j = 0; j < features; ++j) {
                double diff = input.data()[i * features + j] - batch_mean_.data()[j];
                batch_var_.data()[j] += diff * diff;
            }
        for (size_t j = 0; j < features; ++j)
            batch_var_.data()[j] /= batch;
        
        // Update running stats
        for (size_t j = 0; j < features; ++j) {
            running_mean_.data()[j] = (1.0 - momentum_) * running_mean_.data()[j] + momentum_ * batch_mean_.data()[j];
            running_var_.data()[j] = (1.0 - momentum_) * running_var_.data()[j] + momentum_ * batch_var_.data()[j];
        }
        
        // Normalize and apply scale/shift
        for (size_t i = 0; i < batch; ++i)
            for (size_t j = 0; j < features; ++j) {
                normalized_.data()[i * features + j] = 
                    (input.data()[i * features + j] - batch_mean_.data()[j]) / std::sqrt(batch_var_.data()[j] + epsilon_);
                output.data()[i * features + j] = gamma_.data()[j] * normalized_.data()[i * features + j] + beta_.data()[j];
            }
    } else {
        for (size_t i = 0; i < batch; ++i)
            for (size_t j = 0; j < features; ++j) {
                normalized_.data()[i * features + j] = 
                    (input.data()[i * features + j] - running_mean_.data()[j]) / std::sqrt(running_var_.data()[j] + epsilon_);
                output.data()[i * features + j] = gamma_.data()[j] * normalized_.data()[i * features + j] + beta_.data()[j];
            }
    }
    
    last_output_ = output;
    return output;
}

Tensor BatchNormLayer::backward(const Tensor& gradient) {
    size_t batch = gradient.shape()[0];
    size_t features = num_features_;
    
    gamma_gradient_ = Tensor({features}, 0.0);
    beta_gradient_ = Tensor({features}, 0.0);
    
    for (size_t i = 0; i < batch; ++i)
        for (size_t j = 0; j < features; ++j) {
            gamma_gradient_.data()[j] += gradient.data()[i * features + j] * normalized_.data()[i * features + j];
            beta_gradient_.data()[j] += gradient.data()[i * features + j];
        }
    
    Tensor input_grad(gradient.shape(), 0.0);
    double N = static_cast<double>(batch);
    for (size_t j = 0; j < features; ++j) {
        double inv_std = 1.0 / std::sqrt(batch_var_.data()[j] + epsilon_);
        for (size_t i = 0; i < batch; ++i) {
            input_grad.data()[i * features + j] = gamma_.data()[j] * inv_std / N *
                (N * gradient.data()[i * features + j] 
                 - beta_gradient_.data()[j] 
                 - normalized_.data()[i * features + j] * gamma_gradient_.data()[j]);
        }
    }
    return input_grad;
}

void BatchNormLayer::update_parameters(double learning_rate) {
    for (size_t i = 0; i < num_features_; ++i) {
        gamma_.data()[i] -= learning_rate * gamma_gradient_.data()[i];
        beta_.data()[i] -= learning_rate * beta_gradient_.data()[i];
    }
}

// LayerNormLayer
LayerNormLayer::LayerNormLayer(size_t normalized_shape, double epsilon)
    : normalized_shape_(normalized_shape), epsilon_(epsilon) {
    gamma_ = Tensor({normalized_shape}, 1.0);
    beta_ = Tensor({normalized_shape}, 0.0);
    gamma_gradient_ = Tensor({normalized_shape}, 0.0);
    beta_gradient_ = Tensor({normalized_shape}, 0.0);
}

Tensor LayerNormLayer::forward(const Tensor& input) {
    last_input_ = input;
    size_t total = input.size();
    size_t features = normalized_shape_;
    size_t num_elements = total / features;
    
    Tensor output(input.shape(), 0.0);
    normalized_ = Tensor(input.shape(), 0.0);
    std_inv_ = Tensor({num_elements}, 0.0);
    
    for (size_t i = 0; i < num_elements; ++i) {
        // Compute mean over features
        double mean = 0.0;
        for (size_t j = 0; j < features; ++j)
            mean += input.data()[i * features + j];
        mean /= features;
        
        // Compute variance
        double var = 0.0;
        for (size_t j = 0; j < features; ++j) {
            double diff = input.data()[i * features + j] - mean;
            var += diff * diff;
        }
        var /= features;
        
        double inv_std = 1.0 / std::sqrt(var + epsilon_);
        std_inv_.data()[i] = inv_std;
        
        for (size_t j = 0; j < features; ++j) {
            normalized_.data()[i * features + j] = (input.data()[i * features + j] - mean) * inv_std;
            output.data()[i * features + j] = gamma_.data()[j] * normalized_.data()[i * features + j] + beta_.data()[j];
        }
    }
    
    last_output_ = output;
    return output;
}

Tensor LayerNormLayer::backward(const Tensor& gradient) {
    size_t total = gradient.size();
    size_t features = normalized_shape_;
    size_t num_elements = total / features;
    
    gamma_gradient_ = Tensor({features}, 0.0);
    beta_gradient_ = Tensor({features}, 0.0);
    
    for (size_t i = 0; i < num_elements; ++i)
        for (size_t j = 0; j < features; ++j) {
            gamma_gradient_.data()[j] += gradient.data()[i * features + j] * normalized_.data()[i * features + j];
            beta_gradient_.data()[j] += gradient.data()[i * features + j];
        }
    
    Tensor input_grad(gradient.shape(), 0.0);
    double N = static_cast<double>(features);
    
    for (size_t i = 0; i < num_elements; ++i) {
        double sum_dg = 0.0, sum_dg_norm = 0.0;
        for (size_t j = 0; j < features; ++j) {
            double dy = gradient.data()[i * features + j] * gamma_.data()[j];
            sum_dg += dy;
            sum_dg_norm += dy * normalized_.data()[i * features + j];
        }
        for (size_t j = 0; j < features; ++j) {
            double dy = gradient.data()[i * features + j] * gamma_.data()[j];
            input_grad.data()[i * features + j] = std_inv_.data()[i] / N *
                (N * dy - sum_dg - normalized_.data()[i * features + j] * sum_dg_norm);
        }
    }
    return input_grad;
}

void LayerNormLayer::update_parameters(double learning_rate) {
    for (size_t i = 0; i < normalized_shape_; ++i) {
        gamma_.data()[i] -= learning_rate * gamma_gradient_.data()[i];
        beta_.data()[i] -= learning_rate * beta_gradient_.data()[i];
    }
}

// FlattenLayer
Tensor FlattenLayer::forward(const Tensor& input) {
    last_input_ = input;
    original_shape_ = input.shape();
    size_t batch = input.shape()[0];
    size_t flat_size = input.size() / batch;
    return input.reshape({batch, flat_size});
}

Tensor FlattenLayer::backward(const Tensor& gradient) {
    return gradient.reshape(original_shape_);
}

// ============================================================================
// Convolutional Layers
// ============================================================================

// Conv1DLayer
Conv1DLayer::Conv1DLayer(size_t in_channels, size_t out_channels, size_t kernel_size,
                         size_t stride, size_t padding)
    : in_channels_(in_channels), out_channels_(out_channels), kernel_size_(kernel_size),
      stride_(stride), padding_(padding) {
    weights_ = Tensor({out_channels, in_channels, kernel_size});
    bias_ = Tensor({out_channels}, 0.0);
    weight_gradient_ = Tensor({out_channels, in_channels, kernel_size}, 0.0);
    bias_gradient_ = Tensor({out_channels}, 0.0);
    
    double limit = std::sqrt(6.0 / (in_channels * kernel_size + out_channels * kernel_size));
    weights_.randomize(-limit, limit);
}

Tensor Conv1DLayer::forward(const Tensor& input) {
    // Input: [batch, in_channels, length]
    last_input_ = input;
    size_t batch = input.shape()[0];
    size_t in_len = input.shape()[2];
    size_t out_len = (in_len + 2 * padding_ - kernel_size_) / stride_ + 1;
    
    Tensor output({batch, out_channels_, out_len}, 0.0);
    
    for (size_t b = 0; b < batch; ++b) {
        for (size_t oc = 0; oc < out_channels_; ++oc) {
            for (size_t ol = 0; ol < out_len; ++ol) {
                double sum = bias_.data()[oc];
                for (size_t ic = 0; ic < in_channels_; ++ic) {
                    for (size_t k = 0; k < kernel_size_; ++k) {
                        int in_pos = static_cast<int>(ol * stride_ + k) - static_cast<int>(padding_);
                        if (in_pos >= 0 && in_pos < static_cast<int>(in_len)) {
                            size_t w_idx = oc * (in_channels_ * kernel_size_) + ic * kernel_size_ + k;
                            size_t i_idx = b * (in_channels_ * in_len) + ic * in_len + in_pos;
                            sum += weights_.data()[w_idx] * input.data()[i_idx];
                        }
                    }
                }
                output.data()[b * (out_channels_ * out_len) + oc * out_len + ol] = sum;
            }
        }
    }
    last_output_ = output;
    return output;
}

Tensor Conv1DLayer::backward(const Tensor& gradient) {
    // gradient: [batch, out_channels, out_len]
    size_t batch = last_input_.shape()[0];
    size_t in_len = last_input_.shape()[2];
    size_t out_len = gradient.shape()[2];
    
    weight_gradient_ = Tensor(weights_.shape(), 0.0);
    bias_gradient_ = Tensor({out_channels_}, 0.0);
    Tensor input_grad(last_input_.shape(), 0.0);
    
    for (size_t b = 0; b < batch; ++b) {
        for (size_t oc = 0; oc < out_channels_; ++oc) {
            for (size_t ol = 0; ol < out_len; ++ol) {
                double grad_val = gradient.data()[b * (out_channels_ * out_len) + oc * out_len + ol];
                bias_gradient_.data()[oc] += grad_val;
                
                for (size_t ic = 0; ic < in_channels_; ++ic) {
                    for (size_t k = 0; k < kernel_size_; ++k) {
                        int in_pos = static_cast<int>(ol * stride_ + k) - static_cast<int>(padding_);
                        if (in_pos >= 0 && in_pos < static_cast<int>(in_len)) {
                            size_t w_idx = oc * (in_channels_ * kernel_size_) + ic * kernel_size_ + k;
                            size_t i_idx = b * (in_channels_ * in_len) + ic * in_len + in_pos;
                            weight_gradient_.data()[w_idx] += grad_val * last_input_.data()[i_idx];
                            input_grad.data()[i_idx] += grad_val * weights_.data()[w_idx];
                        }
                    }
                }
            }
        }
    }
    return input_grad;
}

void Conv1DLayer::update_parameters(double learning_rate) {
    double batch_size = static_cast<double>(last_input_.shape()[0]);
    for (size_t i = 0; i < weights_.size(); ++i)
        weights_.data()[i] -= learning_rate * weight_gradient_.data()[i] / batch_size;
    for (size_t i = 0; i < bias_.size(); ++i)
        bias_.data()[i] -= learning_rate * bias_gradient_.data()[i] / batch_size;
}

// Conv2DLayer
Conv2DLayer::Conv2DLayer(size_t in_channels, size_t out_channels,
                         size_t kernel_h, size_t kernel_w,
                         size_t stride_h, size_t stride_w,
                         size_t pad_h, size_t pad_w)
    : in_channels_(in_channels), out_channels_(out_channels),
      kernel_h_(kernel_h), kernel_w_(kernel_w),
      stride_h_(stride_h), stride_w_(stride_w),
      pad_h_(pad_h), pad_w_(pad_w) {
    weights_ = Tensor({out_channels, in_channels, kernel_h, kernel_w});
    bias_ = Tensor({out_channels}, 0.0);
    weight_gradient_ = Tensor({out_channels, in_channels, kernel_h, kernel_w}, 0.0);
    bias_gradient_ = Tensor({out_channels}, 0.0);
    
    double fan_in = in_channels * kernel_h * kernel_w;
    double fan_out = out_channels * kernel_h * kernel_w;
    double limit = std::sqrt(6.0 / (fan_in + fan_out));
    weights_.randomize(-limit, limit);
}

Conv2DLayer::Conv2DLayer(size_t in_channels, size_t out_channels, size_t kernel_size,
                         size_t stride, size_t padding)
    : Conv2DLayer(in_channels, out_channels, kernel_size, kernel_size, stride, stride, padding, padding) {}

Tensor Conv2DLayer::forward(const Tensor& input) {
    // Input: [batch, in_channels, height, width]
    last_input_ = input;
    size_t batch = input.shape()[0];
    size_t in_h = input.shape()[2];
    size_t in_w = input.shape()[3];
    size_t out_h = (in_h + 2 * pad_h_ - kernel_h_) / stride_h_ + 1;
    size_t out_w = (in_w + 2 * pad_w_ - kernel_w_) / stride_w_ + 1;
    
    Tensor output({batch, out_channels_, out_h, out_w}, 0.0);
    
    for (size_t b = 0; b < batch; ++b) {
        for (size_t oc = 0; oc < out_channels_; ++oc) {
            for (size_t oh = 0; oh < out_h; ++oh) {
                for (size_t ow = 0; ow < out_w; ++ow) {
                    double sum = bias_.data()[oc];
                    for (size_t ic = 0; ic < in_channels_; ++ic) {
                        for (size_t kh = 0; kh < kernel_h_; ++kh) {
                            for (size_t kw = 0; kw < kernel_w_; ++kw) {
                                int ih = static_cast<int>(oh * stride_h_ + kh) - static_cast<int>(pad_h_);
                                int iw = static_cast<int>(ow * stride_w_ + kw) - static_cast<int>(pad_w_);
                                if (ih >= 0 && ih < static_cast<int>(in_h) && iw >= 0 && iw < static_cast<int>(in_w)) {
                                    size_t w_idx = oc * (in_channels_ * kernel_h_ * kernel_w_) +
                                                   ic * (kernel_h_ * kernel_w_) + kh * kernel_w_ + kw;
                                    size_t i_idx = b * (in_channels_ * in_h * in_w) +
                                                   ic * (in_h * in_w) + ih * in_w + iw;
                                    sum += weights_.data()[w_idx] * input.data()[i_idx];
                                }
                            }
                        }
                    }
                    size_t o_idx = b * (out_channels_ * out_h * out_w) +
                                   oc * (out_h * out_w) + oh * out_w + ow;
                    output.data()[o_idx] = sum;
                }
            }
        }
    }
    last_output_ = output;
    return output;
}

Tensor Conv2DLayer::backward(const Tensor& gradient) {
    size_t batch = last_input_.shape()[0];
    size_t in_h = last_input_.shape()[2];
    size_t in_w = last_input_.shape()[3];
    size_t out_h = gradient.shape()[2];
    size_t out_w = gradient.shape()[3];
    
    weight_gradient_ = Tensor(weights_.shape(), 0.0);
    bias_gradient_ = Tensor({out_channels_}, 0.0);
    Tensor input_grad(last_input_.shape(), 0.0);
    
    for (size_t b = 0; b < batch; ++b) {
        for (size_t oc = 0; oc < out_channels_; ++oc) {
            for (size_t oh = 0; oh < out_h; ++oh) {
                for (size_t ow = 0; ow < out_w; ++ow) {
                    size_t o_idx = b * (out_channels_ * out_h * out_w) +
                                   oc * (out_h * out_w) + oh * out_w + ow;
                    double grad_val = gradient.data()[o_idx];
                    bias_gradient_.data()[oc] += grad_val;
                    
                    for (size_t ic = 0; ic < in_channels_; ++ic) {
                        for (size_t kh = 0; kh < kernel_h_; ++kh) {
                            for (size_t kw = 0; kw < kernel_w_; ++kw) {
                                int ih = static_cast<int>(oh * stride_h_ + kh) - static_cast<int>(pad_h_);
                                int iw = static_cast<int>(ow * stride_w_ + kw) - static_cast<int>(pad_w_);
                                if (ih >= 0 && ih < static_cast<int>(in_h) && iw >= 0 && iw < static_cast<int>(in_w)) {
                                    size_t w_idx = oc * (in_channels_ * kernel_h_ * kernel_w_) +
                                                   ic * (kernel_h_ * kernel_w_) + kh * kernel_w_ + kw;
                                    size_t i_idx = b * (in_channels_ * in_h * in_w) +
                                                   ic * (in_h * in_w) + ih * in_w + iw;
                                    weight_gradient_.data()[w_idx] += grad_val * last_input_.data()[i_idx];
                                    input_grad.data()[i_idx] += grad_val * weights_.data()[w_idx];
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return input_grad;
}

void Conv2DLayer::update_parameters(double learning_rate) {
    double batch_size = static_cast<double>(last_input_.shape()[0]);
    for (size_t i = 0; i < weights_.size(); ++i)
        weights_.data()[i] -= learning_rate * weight_gradient_.data()[i] / batch_size;
    for (size_t i = 0; i < bias_.size(); ++i)
        bias_.data()[i] -= learning_rate * bias_gradient_.data()[i] / batch_size;
}

// MaxPool2DLayer
MaxPool2DLayer::MaxPool2DLayer(size_t pool_h, size_t pool_w, size_t stride_h, size_t stride_w)
    : pool_h_(pool_h), pool_w_(pool_w),
      stride_h_(stride_h == 0 ? pool_h : stride_h),
      stride_w_(stride_w == 0 ? pool_w : stride_w),
      input_h_(0), input_w_(0) {}

MaxPool2DLayer::MaxPool2DLayer(size_t pool_size, size_t stride)
    : MaxPool2DLayer(pool_size, pool_size, stride, stride) {}

Tensor MaxPool2DLayer::forward(const Tensor& input) {
    // Input: [batch, channels, height, width]
    last_input_ = input;
    size_t batch = input.shape()[0];
    size_t channels = input.shape()[1];
    input_h_ = input.shape()[2];
    input_w_ = input.shape()[3];
    size_t out_h = (input_h_ - pool_h_) / stride_h_ + 1;
    size_t out_w = (input_w_ - pool_w_) / stride_w_ + 1;
    
    Tensor output({batch, channels, out_h, out_w}, 0.0);
    max_indices_ = Tensor({batch, channels, out_h, out_w}, 0.0);
    
    for (size_t b = 0; b < batch; ++b) {
        for (size_t c = 0; c < channels; ++c) {
            for (size_t oh = 0; oh < out_h; ++oh) {
                for (size_t ow = 0; ow < out_w; ++ow) {
                    double max_val = -std::numeric_limits<double>::infinity();
                    size_t max_idx = 0;
                    for (size_t ph = 0; ph < pool_h_; ++ph) {
                        for (size_t pw = 0; pw < pool_w_; ++pw) {
                            size_t ih = oh * stride_h_ + ph;
                            size_t iw = ow * stride_w_ + pw;
                            size_t idx = b * (channels * input_h_ * input_w_) +
                                         c * (input_h_ * input_w_) + ih * input_w_ + iw;
                            if (input.data()[idx] > max_val) {
                                max_val = input.data()[idx];
                                max_idx = idx;
                            }
                        }
                    }
                    size_t o_idx = b * (channels * out_h * out_w) +
                                   c * (out_h * out_w) + oh * out_w + ow;
                    output.data()[o_idx] = max_val;
                    max_indices_.data()[o_idx] = static_cast<double>(max_idx);
                }
            }
        }
    }
    last_output_ = output;
    return output;
}

Tensor MaxPool2DLayer::backward(const Tensor& gradient) {
    Tensor input_grad(last_input_.shape(), 0.0);
    for (size_t i = 0; i < gradient.size(); ++i) {
        size_t idx = static_cast<size_t>(max_indices_.data()[i]);
        input_grad.data()[idx] += gradient.data()[i];
    }
    return input_grad;
}

// AvgPool2DLayer
AvgPool2DLayer::AvgPool2DLayer(size_t pool_h, size_t pool_w, size_t stride_h, size_t stride_w)
    : pool_h_(pool_h), pool_w_(pool_w),
      stride_h_(stride_h == 0 ? pool_h : stride_h),
      stride_w_(stride_w == 0 ? pool_w : stride_w),
      input_h_(0), input_w_(0), batch_size_(0), channels_(0) {}

AvgPool2DLayer::AvgPool2DLayer(size_t pool_size, size_t stride)
    : AvgPool2DLayer(pool_size, pool_size, stride, stride) {}

Tensor AvgPool2DLayer::forward(const Tensor& input) {
    last_input_ = input;
    batch_size_ = input.shape()[0];
    channels_ = input.shape()[1];
    input_h_ = input.shape()[2];
    input_w_ = input.shape()[3];
    size_t out_h = (input_h_ - pool_h_) / stride_h_ + 1;
    size_t out_w = (input_w_ - pool_w_) / stride_w_ + 1;
    
    Tensor output({batch_size_, channels_, out_h, out_w}, 0.0);
    double pool_area = static_cast<double>(pool_h_ * pool_w_);
    
    for (size_t b = 0; b < batch_size_; ++b) {
        for (size_t c = 0; c < channels_; ++c) {
            for (size_t oh = 0; oh < out_h; ++oh) {
                for (size_t ow = 0; ow < out_w; ++ow) {
                    double sum = 0.0;
                    for (size_t ph = 0; ph < pool_h_; ++ph) {
                        for (size_t pw = 0; pw < pool_w_; ++pw) {
                            size_t ih = oh * stride_h_ + ph;
                            size_t iw = ow * stride_w_ + pw;
                            sum += input.data()[b * (channels_ * input_h_ * input_w_) +
                                                c * (input_h_ * input_w_) + ih * input_w_ + iw];
                        }
                    }
                    output.data()[b * (channels_ * out_h * out_w) +
                                  c * (out_h * out_w) + oh * out_w + ow] = sum / pool_area;
                }
            }
        }
    }
    last_output_ = output;
    return output;
}

Tensor AvgPool2DLayer::backward(const Tensor& gradient) {
    size_t out_h = gradient.shape()[2];
    size_t out_w = gradient.shape()[3];
    double pool_area = static_cast<double>(pool_h_ * pool_w_);
    
    Tensor input_grad(last_input_.shape(), 0.0);
    
    for (size_t b = 0; b < batch_size_; ++b) {
        for (size_t c = 0; c < channels_; ++c) {
            for (size_t oh = 0; oh < out_h; ++oh) {
                for (size_t ow = 0; ow < out_w; ++ow) {
                    double grad_val = gradient.data()[b * (channels_ * out_h * out_w) +
                                                      c * (out_h * out_w) + oh * out_w + ow] / pool_area;
                    for (size_t ph = 0; ph < pool_h_; ++ph) {
                        for (size_t pw = 0; pw < pool_w_; ++pw) {
                            size_t ih = oh * stride_h_ + ph;
                            size_t iw = ow * stride_w_ + pw;
                            input_grad.data()[b * (channels_ * input_h_ * input_w_) +
                                              c * (input_h_ * input_w_) + ih * input_w_ + iw] += grad_val;
                        }
                    }
                }
            }
        }
    }
    return input_grad;
}

// ============================================================================
// Recurrent Layers
// ============================================================================

// RNNLayer
RNNLayer::RNNLayer(size_t input_size, size_t hidden_size, bool return_sequences)
    : input_size_(input_size), hidden_size_(hidden_size), return_sequences_(return_sequences) {
    W_ih_ = Tensor({input_size, hidden_size});
    W_hh_ = Tensor({hidden_size, hidden_size});
    b_h_ = Tensor({hidden_size}, 0.0);
    
    double limit_ih = std::sqrt(6.0 / (input_size + hidden_size));
    double limit_hh = std::sqrt(6.0 / (hidden_size + hidden_size));
    W_ih_.randomize(-limit_ih, limit_ih);
    W_hh_.randomize(-limit_hh, limit_hh);
    
    W_ih_grad_ = Tensor({input_size, hidden_size}, 0.0);
    W_hh_grad_ = Tensor({hidden_size, hidden_size}, 0.0);
    b_h_grad_ = Tensor({hidden_size}, 0.0);
}

void RNNLayer::reset_hidden_state() {
    hidden_state_ = Tensor();
    hidden_states_.clear();
    inputs_.clear();
}

Tensor RNNLayer::forward(const Tensor& input) {
    // Input: [batch, seq_len, input_size]
    last_input_ = input;
    size_t batch = input.shape()[0];
    size_t seq_len = input.shape()[1];
    
    hidden_states_.clear();
    inputs_.clear();
    
    if (hidden_state_.size() == 0) {
        hidden_state_ = Tensor({batch, hidden_size_}, 0.0);
    }
    hidden_states_.push_back(hidden_state_.clone());
    
    Tensor all_outputs({batch, seq_len, hidden_size_}, 0.0);
    
    for (size_t t = 0; t < seq_len; ++t) {
        // Extract input at timestep t: [batch, input_size]
        Tensor x_t({batch, input_size_}, 0.0);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < input_size_; ++j)
                x_t.data()[b * input_size_ + j] = input.data()[b * (seq_len * input_size_) + t * input_size_ + j];
        inputs_.push_back(x_t);
        
        // h_t = tanh(x_t * W_ih + h_{t-1} * W_hh + b_h)
        Tensor pre_act = x_t.matmul(W_ih_);
        Tensor h_contrib = hidden_state_.matmul(W_hh_);
        
        Tensor h_new({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i) {
            h_new.data()[i] = std::tanh(pre_act.data()[i] + h_contrib.data()[i] + b_h_.data()[i % hidden_size_]);
        }
        
        hidden_state_ = h_new;
        hidden_states_.push_back(h_new.clone());
        
        // Store in output
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                all_outputs.data()[b * (seq_len * hidden_size_) + t * hidden_size_ + j] = h_new.data()[b * hidden_size_ + j];
    }
    
    if (return_sequences_) {
        last_output_ = all_outputs;
        return all_outputs;
    } else {
        // Return only last timestep: [batch, hidden_size]
        Tensor last({batch, hidden_size_}, 0.0);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                last.data()[b * hidden_size_ + j] = all_outputs.data()[b * (seq_len * hidden_size_) + (seq_len - 1) * hidden_size_ + j];
        last_output_ = last;
        return last;
    }
}

Tensor RNNLayer::backward(const Tensor& gradient) {
    size_t batch = last_input_.shape()[0];
    size_t seq_len = last_input_.shape()[1];
    
    W_ih_grad_ = Tensor({input_size_, hidden_size_}, 0.0);
    W_hh_grad_ = Tensor({hidden_size_, hidden_size_}, 0.0);
    b_h_grad_ = Tensor({hidden_size_}, 0.0);
    
    Tensor input_grad(last_input_.shape(), 0.0);
    Tensor dh_next({batch, hidden_size_}, 0.0);
    
    for (int t = static_cast<int>(seq_len) - 1; t >= 0; --t) {
        // Get gradient for this timestep
        Tensor dh_t({batch, hidden_size_}, 0.0);
        if (return_sequences_) {
            for (size_t b = 0; b < batch; ++b)
                for (size_t j = 0; j < hidden_size_; ++j)
                    dh_t.data()[b * hidden_size_ + j] = gradient.data()[b * (seq_len * hidden_size_) + t * hidden_size_ + j];
        } else if (t == static_cast<int>(seq_len) - 1) {
            dh_t = gradient.clone();
        }
        
        // Add gradient from next timestep
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            dh_t.data()[i] += dh_next.data()[i];
        
        // tanh derivative: dtanh = (1 - h_t^2) * dh_t
        const Tensor& h_t = hidden_states_[t + 1];
        Tensor dtanh({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i) {
            double h = h_t.data()[i];
            dtanh.data()[i] = dh_t.data()[i] * (1.0 - h * h);
        }
        
        // Gradient w.r.t. W_ih: x_t^T * dtanh
        Tensor w_ih_grad = inputs_[t].transpose().matmul(dtanh);
        for (size_t i = 0; i < W_ih_grad_.size(); ++i)
            W_ih_grad_.data()[i] += w_ih_grad.data()[i];
        
        // Gradient w.r.t. W_hh: h_{t-1}^T * dtanh
        const Tensor& h_prev = hidden_states_[t];
        Tensor w_hh_grad = h_prev.transpose().matmul(dtanh);
        for (size_t i = 0; i < W_hh_grad_.size(); ++i)
            W_hh_grad_.data()[i] += w_hh_grad.data()[i];
        
        // Gradient w.r.t. bias
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                b_h_grad_.data()[j] += dtanh.data()[b * hidden_size_ + j];
        
        // Gradient w.r.t. input
        Tensor dx = dtanh.matmul(W_ih_.transpose());
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < input_size_; ++j)
                input_grad.data()[b * (seq_len * input_size_) + t * input_size_ + j] = dx.data()[b * input_size_ + j];
        
        // Gradient to propagate to previous timestep
        dh_next = dtanh.matmul(W_hh_.transpose());
    }
    
    return input_grad;
}

void RNNLayer::update_parameters(double learning_rate) {
    double batch_size = static_cast<double>(last_input_.shape()[0]);
    for (size_t i = 0; i < W_ih_.size(); ++i)
        W_ih_.data()[i] -= learning_rate * W_ih_grad_.data()[i] / batch_size;
    for (size_t i = 0; i < W_hh_.size(); ++i)
        W_hh_.data()[i] -= learning_rate * W_hh_grad_.data()[i] / batch_size;
    for (size_t i = 0; i < b_h_.size(); ++i)
        b_h_.data()[i] -= learning_rate * b_h_grad_.data()[i] / batch_size;
}

// LSTMLayer
LSTMLayer::LSTMLayer(size_t input_size, size_t hidden_size, bool return_sequences)
    : input_size_(input_size), hidden_size_(hidden_size), return_sequences_(return_sequences) {
    size_t combined = input_size + hidden_size;
    double limit = std::sqrt(6.0 / (combined + hidden_size));
    
    W_f_ = Tensor({combined, hidden_size}); W_f_.randomize(-limit, limit);
    b_f_ = Tensor({hidden_size}, 1.0); // Bias init to 1 for forget gate
    W_i_ = Tensor({combined, hidden_size}); W_i_.randomize(-limit, limit);
    b_i_ = Tensor({hidden_size}, 0.0);
    W_c_ = Tensor({combined, hidden_size}); W_c_.randomize(-limit, limit);
    b_c_ = Tensor({hidden_size}, 0.0);
    W_o_ = Tensor({combined, hidden_size}); W_o_.randomize(-limit, limit);
    b_o_ = Tensor({hidden_size}, 0.0);
    
    W_f_grad_ = Tensor({combined, hidden_size}, 0.0); b_f_grad_ = Tensor({hidden_size}, 0.0);
    W_i_grad_ = Tensor({combined, hidden_size}, 0.0); b_i_grad_ = Tensor({hidden_size}, 0.0);
    W_c_grad_ = Tensor({combined, hidden_size}, 0.0); b_c_grad_ = Tensor({hidden_size}, 0.0);
    W_o_grad_ = Tensor({combined, hidden_size}, 0.0); b_o_grad_ = Tensor({hidden_size}, 0.0);
}

void LSTMLayer::reset_hidden_state() {
    hidden_state_ = Tensor();
    cell_state_ = Tensor();
    forget_gates_.clear();
    input_gates_.clear();
    cell_candidates_.clear();
    output_gates_.clear();
    cell_states_.clear();
    hidden_states_.clear();
    inputs_.clear();
}

Tensor LSTMLayer::forward(const Tensor& input) {
    last_input_ = input;
    size_t batch = input.shape()[0];
    size_t seq_len = input.shape()[1];
    
    forget_gates_.clear();
    input_gates_.clear();
    cell_candidates_.clear();
    output_gates_.clear();
    cell_states_.clear();
    hidden_states_.clear();
    inputs_.clear();
    
    if (hidden_state_.size() == 0) {
        hidden_state_ = Tensor({batch, hidden_size_}, 0.0);
        cell_state_ = Tensor({batch, hidden_size_}, 0.0);
    }
    hidden_states_.push_back(hidden_state_.clone());
    cell_states_.push_back(cell_state_.clone());
    
    Tensor all_outputs({batch, seq_len, hidden_size_}, 0.0);
    
    for (size_t t = 0; t < seq_len; ++t) {
        // Extract x_t
        Tensor x_t({batch, input_size_}, 0.0);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < input_size_; ++j)
                x_t.data()[b * input_size_ + j] = input.data()[b * (seq_len * input_size_) + t * input_size_ + j];
        
        // Concatenate [x_t, h_{t-1}]
        Tensor combined = concat_cols(x_t, hidden_state_);
        inputs_.push_back(combined);
        
        // Forget gate: f_t = sigmoid(combined * W_f + b_f)
        Tensor f_pre = combined.matmul(W_f_);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                f_pre.data()[b * hidden_size_ + j] += b_f_.data()[j];
        Tensor f_t = apply_sigmoid(f_pre);
        forget_gates_.push_back(f_t);
        
        // Input gate: i_t = sigmoid(combined * W_i + b_i)
        Tensor i_pre = combined.matmul(W_i_);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                i_pre.data()[b * hidden_size_ + j] += b_i_.data()[j];
        Tensor i_t = apply_sigmoid(i_pre);
        input_gates_.push_back(i_t);
        
        // Cell candidate: c_hat = tanh(combined * W_c + b_c)
        Tensor c_pre = combined.matmul(W_c_);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                c_pre.data()[b * hidden_size_ + j] += b_c_.data()[j];
        Tensor c_hat = apply_tanh(c_pre);
        cell_candidates_.push_back(c_hat);
        
        // Output gate: o_t = sigmoid(combined * W_o + b_o)
        Tensor o_pre = combined.matmul(W_o_);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                o_pre.data()[b * hidden_size_ + j] += b_o_.data()[j];
        Tensor o_t = apply_sigmoid(o_pre);
        output_gates_.push_back(o_t);
        
        // Cell state: c_t = f_t * c_{t-1} + i_t * c_hat
        Tensor c_t({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            c_t.data()[i] = f_t.data()[i] * cell_state_.data()[i] + i_t.data()[i] * c_hat.data()[i];
        cell_state_ = c_t;
        cell_states_.push_back(c_t.clone());
        
        // Hidden state: h_t = o_t * tanh(c_t)
        Tensor tanh_c = apply_tanh(c_t);
        Tensor h_t({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            h_t.data()[i] = o_t.data()[i] * tanh_c.data()[i];
        hidden_state_ = h_t;
        hidden_states_.push_back(h_t.clone());
        
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                all_outputs.data()[b * (seq_len * hidden_size_) + t * hidden_size_ + j] = h_t.data()[b * hidden_size_ + j];
    }
    
    if (return_sequences_) {
        last_output_ = all_outputs;
        return all_outputs;
    } else {
        Tensor last({batch, hidden_size_}, 0.0);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                last.data()[b * hidden_size_ + j] = all_outputs.data()[b * (seq_len * hidden_size_) + (seq_len - 1) * hidden_size_ + j];
        last_output_ = last;
        return last;
    }
}

Tensor LSTMLayer::backward(const Tensor& gradient) {
    size_t batch = last_input_.shape()[0];
    size_t seq_len = last_input_.shape()[1];
    size_t combined_size = input_size_ + hidden_size_;
    
    W_f_grad_ = Tensor({combined_size, hidden_size_}, 0.0); b_f_grad_ = Tensor({hidden_size_}, 0.0);
    W_i_grad_ = Tensor({combined_size, hidden_size_}, 0.0); b_i_grad_ = Tensor({hidden_size_}, 0.0);
    W_c_grad_ = Tensor({combined_size, hidden_size_}, 0.0); b_c_grad_ = Tensor({hidden_size_}, 0.0);
    W_o_grad_ = Tensor({combined_size, hidden_size_}, 0.0); b_o_grad_ = Tensor({hidden_size_}, 0.0);
    
    Tensor input_grad(last_input_.shape(), 0.0);
    Tensor dh_next({batch, hidden_size_}, 0.0);
    Tensor dc_next({batch, hidden_size_}, 0.0);
    
    for (int t = static_cast<int>(seq_len) - 1; t >= 0; --t) {
        Tensor dh_t({batch, hidden_size_}, 0.0);
        if (return_sequences_) {
            for (size_t b = 0; b < batch; ++b)
                for (size_t j = 0; j < hidden_size_; ++j)
                    dh_t.data()[b * hidden_size_ + j] = gradient.data()[b * (seq_len * hidden_size_) + t * hidden_size_ + j];
        } else if (t == static_cast<int>(seq_len) - 1) {
            dh_t = gradient.clone();
        }
        
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            dh_t.data()[i] += dh_next.data()[i];
        
        const Tensor& o_t = output_gates_[t];
        const Tensor& c_t = cell_states_[t + 1];
        const Tensor& f_t = forget_gates_[t];
        const Tensor& i_t = input_gates_[t];
        const Tensor& c_hat = cell_candidates_[t];
        const Tensor& c_prev = cell_states_[t];
        
        Tensor tanh_c = apply_tanh(c_t);
        
        // dh -> do, dc
        Tensor do_t({batch, hidden_size_}, 0.0);
        Tensor dc_t({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i) {
            do_t.data()[i] = dh_t.data()[i] * tanh_c.data()[i];
            dc_t.data()[i] = dh_t.data()[i] * o_t.data()[i] * (1.0 - tanh_c.data()[i] * tanh_c.data()[i]) + dc_next.data()[i];
        }
        
        // Gate gradients (pre-activation)
        Tensor df_t({batch, hidden_size_}, 0.0);
        Tensor di_t({batch, hidden_size_}, 0.0);
        Tensor dc_hat({batch, hidden_size_}, 0.0);
        Tensor do_pre({batch, hidden_size_}, 0.0);
        
        for (size_t i = 0; i < batch * hidden_size_; ++i) {
            df_t.data()[i] = dc_t.data()[i] * c_prev.data()[i] * f_t.data()[i] * (1.0 - f_t.data()[i]);
            di_t.data()[i] = dc_t.data()[i] * c_hat.data()[i] * i_t.data()[i] * (1.0 - i_t.data()[i]);
            dc_hat.data()[i] = dc_t.data()[i] * i_t.data()[i] * (1.0 - c_hat.data()[i] * c_hat.data()[i]);
            do_pre.data()[i] = do_t.data()[i] * o_t.data()[i] * (1.0 - o_t.data()[i]);
        }
        
        // Accumulate weight gradients
        const Tensor& combined = inputs_[t];
        Tensor combined_T = combined.transpose();
        
        Tensor wf_g = combined_T.matmul(df_t);
        Tensor wi_g = combined_T.matmul(di_t);
        Tensor wc_g = combined_T.matmul(dc_hat);
        Tensor wo_g = combined_T.matmul(do_pre);
        
        for (size_t i = 0; i < W_f_grad_.size(); ++i) {
            W_f_grad_.data()[i] += wf_g.data()[i];
            W_i_grad_.data()[i] += wi_g.data()[i];
            W_c_grad_.data()[i] += wc_g.data()[i];
            W_o_grad_.data()[i] += wo_g.data()[i];
        }
        
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j) {
                b_f_grad_.data()[j] += df_t.data()[b * hidden_size_ + j];
                b_i_grad_.data()[j] += di_t.data()[b * hidden_size_ + j];
                b_c_grad_.data()[j] += dc_hat.data()[b * hidden_size_ + j];
                b_o_grad_.data()[j] += do_pre.data()[b * hidden_size_ + j];
            }
        
        // Gradient w.r.t. combined input [x_t, h_{t-1}]
        Tensor d_combined({batch, combined_size}, 0.0);
        Tensor dc_f = df_t.matmul(W_f_.transpose());
        Tensor dc_i = di_t.matmul(W_i_.transpose());
        Tensor dc_c = dc_hat.matmul(W_c_.transpose());
        Tensor dc_o = do_pre.matmul(W_o_.transpose());
        
        for (size_t i = 0; i < batch * combined_size; ++i)
            d_combined.data()[i] = dc_f.data()[i] + dc_i.data()[i] + dc_c.data()[i] + dc_o.data()[i];
        
        // Split d_combined into dx and dh_prev
        for (size_t b = 0; b < batch; ++b) {
            for (size_t j = 0; j < input_size_; ++j)
                input_grad.data()[b * (seq_len * input_size_) + t * input_size_ + j] = d_combined.data()[b * combined_size + j];
            for (size_t j = 0; j < hidden_size_; ++j)
                dh_next.data()[b * hidden_size_ + j] = d_combined.data()[b * combined_size + input_size_ + j];
        }
        
        // dc_next = dc_t * f_t
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            dc_next.data()[i] = dc_t.data()[i] * f_t.data()[i];
    }
    
    return input_grad;
}

void LSTMLayer::update_parameters(double learning_rate) {
    double bs = static_cast<double>(last_input_.shape()[0]);
    auto update = [&](Tensor& W, Tensor& grad, Tensor& b, Tensor& b_grad) {
        for (size_t i = 0; i < W.size(); ++i)
            W.data()[i] -= learning_rate * grad.data()[i] / bs;
        for (size_t i = 0; i < b.size(); ++i)
            b.data()[i] -= learning_rate * b_grad.data()[i] / bs;
    };
    update(W_f_, W_f_grad_, b_f_, b_f_grad_);
    update(W_i_, W_i_grad_, b_i_, b_i_grad_);
    update(W_c_, W_c_grad_, b_c_, b_c_grad_);
    update(W_o_, W_o_grad_, b_o_, b_o_grad_);
}

// GRULayer
GRULayer::GRULayer(size_t input_size, size_t hidden_size, bool return_sequences)
    : input_size_(input_size), hidden_size_(hidden_size), return_sequences_(return_sequences) {
    size_t combined = input_size + hidden_size;
    double limit = std::sqrt(6.0 / (combined + hidden_size));
    
    W_r_ = Tensor({combined, hidden_size}); W_r_.randomize(-limit, limit);
    b_r_ = Tensor({hidden_size}, 0.0);
    W_z_ = Tensor({combined, hidden_size}); W_z_.randomize(-limit, limit);
    b_z_ = Tensor({hidden_size}, 0.0);
    W_n_ = Tensor({combined, hidden_size}); W_n_.randomize(-limit, limit);
    b_n_ = Tensor({hidden_size}, 0.0);
    
    W_r_grad_ = Tensor({combined, hidden_size}, 0.0); b_r_grad_ = Tensor({hidden_size}, 0.0);
    W_z_grad_ = Tensor({combined, hidden_size}, 0.0); b_z_grad_ = Tensor({hidden_size}, 0.0);
    W_n_grad_ = Tensor({combined, hidden_size}, 0.0); b_n_grad_ = Tensor({hidden_size}, 0.0);
}

void GRULayer::reset_hidden_state() {
    hidden_state_ = Tensor();
    reset_gates_.clear();
    update_gates_.clear();
    candidates_.clear();
    hidden_states_.clear();
    inputs_.clear();
}

Tensor GRULayer::forward(const Tensor& input) {
    last_input_ = input;
    size_t batch = input.shape()[0];
    size_t seq_len = input.shape()[1];
    
    reset_gates_.clear();
    update_gates_.clear();
    candidates_.clear();
    hidden_states_.clear();
    inputs_.clear();
    
    if (hidden_state_.size() == 0) {
        hidden_state_ = Tensor({batch, hidden_size_}, 0.0);
    }
    hidden_states_.push_back(hidden_state_.clone());
    
    Tensor all_outputs({batch, seq_len, hidden_size_}, 0.0);
    
    for (size_t t = 0; t < seq_len; ++t) {
        Tensor x_t({batch, input_size_}, 0.0);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < input_size_; ++j)
                x_t.data()[b * input_size_ + j] = input.data()[b * (seq_len * input_size_) + t * input_size_ + j];
        
        Tensor combined = concat_cols(x_t, hidden_state_);
        inputs_.push_back(combined);
        
        // Reset gate: r_t = sigmoid(combined * W_r + b_r)
        Tensor r_pre = combined.matmul(W_r_);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                r_pre.data()[b * hidden_size_ + j] += b_r_.data()[j];
        Tensor r_t = apply_sigmoid(r_pre);
        reset_gates_.push_back(r_t);
        
        // Update gate: z_t = sigmoid(combined * W_z + b_z)
        Tensor z_pre = combined.matmul(W_z_);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                z_pre.data()[b * hidden_size_ + j] += b_z_.data()[j];
        Tensor z_t = apply_sigmoid(z_pre);
        update_gates_.push_back(z_t);
        
        // Candidate: n_t = tanh([x_t, r_t * h_{t-1}] * W_n + b_n)
        Tensor rh({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            rh.data()[i] = r_t.data()[i] * hidden_state_.data()[i];
        Tensor combined_n = concat_cols(x_t, rh);
        Tensor n_pre = combined_n.matmul(W_n_);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                n_pre.data()[b * hidden_size_ + j] += b_n_.data()[j];
        Tensor n_t = apply_tanh(n_pre);
        candidates_.push_back(n_t);
        
        // h_t = (1 - z_t) * n_t + z_t * h_{t-1}
        Tensor h_new({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            h_new.data()[i] = (1.0 - z_t.data()[i]) * n_t.data()[i] + z_t.data()[i] * hidden_state_.data()[i];
        
        hidden_state_ = h_new;
        hidden_states_.push_back(h_new.clone());
        
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                all_outputs.data()[b * (seq_len * hidden_size_) + t * hidden_size_ + j] = h_new.data()[b * hidden_size_ + j];
    }
    
    if (return_sequences_) {
        last_output_ = all_outputs;
        return all_outputs;
    } else {
        Tensor last({batch, hidden_size_}, 0.0);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                last.data()[b * hidden_size_ + j] = all_outputs.data()[b * (seq_len * hidden_size_) + (seq_len - 1) * hidden_size_ + j];
        last_output_ = last;
        return last;
    }
}

Tensor GRULayer::backward(const Tensor& gradient) {
    size_t batch = last_input_.shape()[0];
    size_t seq_len = last_input_.shape()[1];
    size_t combined_size = input_size_ + hidden_size_;
    
    W_r_grad_ = Tensor({combined_size, hidden_size_}, 0.0); b_r_grad_ = Tensor({hidden_size_}, 0.0);
    W_z_grad_ = Tensor({combined_size, hidden_size_}, 0.0); b_z_grad_ = Tensor({hidden_size_}, 0.0);
    W_n_grad_ = Tensor({combined_size, hidden_size_}, 0.0); b_n_grad_ = Tensor({hidden_size_}, 0.0);
    
    Tensor input_grad(last_input_.shape(), 0.0);
    Tensor dh_next({batch, hidden_size_}, 0.0);
    
    for (int t = static_cast<int>(seq_len) - 1; t >= 0; --t) {
        Tensor dh_t({batch, hidden_size_}, 0.0);
        if (return_sequences_) {
            for (size_t b = 0; b < batch; ++b)
                for (size_t j = 0; j < hidden_size_; ++j)
                    dh_t.data()[b * hidden_size_ + j] = gradient.data()[b * (seq_len * hidden_size_) + t * hidden_size_ + j];
        } else if (t == static_cast<int>(seq_len) - 1) {
            dh_t = gradient.clone();
        }
        
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            dh_t.data()[i] += dh_next.data()[i];
        
        const Tensor& z_t = update_gates_[t];
        const Tensor& r_t = reset_gates_[t];
        const Tensor& n_t = candidates_[t];
        const Tensor& h_prev = hidden_states_[t];
        
        // dz = dh * (h_prev - n_t)
        // dn = dh * (1 - z_t)
        Tensor dz({batch, hidden_size_}, 0.0);
        Tensor dn({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i) {
            dz.data()[i] = dh_t.data()[i] * (h_prev.data()[i] - n_t.data()[i]);
            dn.data()[i] = dh_t.data()[i] * (1.0 - z_t.data()[i]);
        }
        
        // dn through tanh: dn_pre = dn * (1 - n_t^2)
        Tensor dn_pre({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            dn_pre.data()[i] = dn.data()[i] * (1.0 - n_t.data()[i] * n_t.data()[i]);
        
        // dz through sigmoid: dz_pre = dz * z_t * (1 - z_t)
        Tensor dz_pre({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            dz_pre.data()[i] = dz.data()[i] * z_t.data()[i] * (1.0 - z_t.data()[i]);
        
        // Gradient for W_n: [x_t, r_t * h_prev]^T * dn_pre
        Tensor rh({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            rh.data()[i] = r_t.data()[i] * h_prev.data()[i];
        
        // Extract x_t from combined input
        Tensor x_t({batch, input_size_}, 0.0);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < input_size_; ++j)
                x_t.data()[b * input_size_ + j] = inputs_[t].data()[b * combined_size + j];
        
        Tensor combined_n = concat_cols(x_t, rh);
        Tensor wn_g = combined_n.transpose().matmul(dn_pre);
        for (size_t i = 0; i < W_n_grad_.size(); ++i)
            W_n_grad_.data()[i] += wn_g.data()[i];
        
        // Gradient for W_z and W_r
        const Tensor& combined = inputs_[t];
        Tensor wz_g = combined.transpose().matmul(dz_pre);
        for (size_t i = 0; i < W_z_grad_.size(); ++i)
            W_z_grad_.data()[i] += wz_g.data()[i];
        
        // dr from dn_pre through W_n and the reset gate
        Tensor d_combined_n = dn_pre.matmul(W_n_.transpose());
        Tensor dr_h({batch, hidden_size_}, 0.0);
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j)
                dr_h.data()[b * hidden_size_ + j] = d_combined_n.data()[b * combined_size + input_size_ + j];
        
        // dr = dr_h * h_prev, then through sigmoid
        Tensor dr_pre({batch, hidden_size_}, 0.0);
        for (size_t i = 0; i < batch * hidden_size_; ++i)
            dr_pre.data()[i] = dr_h.data()[i] * h_prev.data()[i] * r_t.data()[i] * (1.0 - r_t.data()[i]);
        
        Tensor wr_g = combined.transpose().matmul(dr_pre);
        for (size_t i = 0; i < W_r_grad_.size(); ++i)
            W_r_grad_.data()[i] += wr_g.data()[i];
        
        // Bias gradients
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j) {
                b_r_grad_.data()[j] += dr_pre.data()[b * hidden_size_ + j];
                b_z_grad_.data()[j] += dz_pre.data()[b * hidden_size_ + j];
                b_n_grad_.data()[j] += dn_pre.data()[b * hidden_size_ + j];
            }
        
        // Gradient w.r.t. combined input for W_z and W_r
        Tensor d_combined_z = dz_pre.matmul(W_z_.transpose());
        Tensor d_combined_r = dr_pre.matmul(W_r_.transpose());
        
        // dx from all sources
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < input_size_; ++j) {
                double dx = d_combined_z.data()[b * combined_size + j] +
                            d_combined_r.data()[b * combined_size + j] +
                            d_combined_n.data()[b * combined_size + j];
                input_grad.data()[b * (seq_len * input_size_) + t * input_size_ + j] = dx;
            }
        
        // dh_next from z gate, reset gate influence, and candidate
        for (size_t b = 0; b < batch; ++b)
            for (size_t j = 0; j < hidden_size_; ++j) {
                dh_next.data()[b * hidden_size_ + j] =
                    dh_t.data()[b * hidden_size_ + j] * z_t.data()[b * hidden_size_ + j] +
                    d_combined_z.data()[b * combined_size + input_size_ + j] +
                    d_combined_r.data()[b * combined_size + input_size_ + j] +
                    dr_h.data()[b * hidden_size_ + j] * r_t.data()[b * hidden_size_ + j];
            }
    }
    
    return input_grad;
}

void GRULayer::update_parameters(double learning_rate) {
    double bs = static_cast<double>(last_input_.shape()[0]);
    auto update = [&](Tensor& W, Tensor& grad, Tensor& b, Tensor& b_grad) {
        for (size_t i = 0; i < W.size(); ++i)
            W.data()[i] -= learning_rate * grad.data()[i] / bs;
        for (size_t i = 0; i < b.size(); ++i)
            b.data()[i] -= learning_rate * b_grad.data()[i] / bs;
    };
    update(W_r_, W_r_grad_, b_r_, b_r_grad_);
    update(W_z_, W_z_grad_, b_z_, b_z_grad_);
    update(W_n_, W_n_grad_, b_n_, b_n_grad_);
}

// EmbeddingLayer
EmbeddingLayer::EmbeddingLayer(size_t vocab_size, size_t embedding_dim)
    : vocab_size_(vocab_size), embedding_dim_(embedding_dim) {
    embeddings_ = Tensor({vocab_size, embedding_dim});
    embeddings_.randomize(-0.1, 0.1);
    embedding_grad_ = Tensor({vocab_size, embedding_dim}, 0.0);
}

Tensor EmbeddingLayer::forward(const Tensor& input) {
    // Input: [batch, seq_len] with integer indices stored as doubles
    last_input_ = input;
    size_t batch = input.shape()[0];
    size_t seq_len = input.shape()[1];
    
    Tensor output({batch, seq_len, embedding_dim_}, 0.0);
    
    for (size_t b = 0; b < batch; ++b) {
        for (size_t t = 0; t < seq_len; ++t) {
            size_t idx = static_cast<size_t>(input.data()[b * seq_len + t]);
            for (size_t d = 0; d < embedding_dim_; ++d) {
                output.data()[b * (seq_len * embedding_dim_) + t * embedding_dim_ + d] =
                    embeddings_.data()[idx * embedding_dim_ + d];
            }
        }
    }
    last_output_ = output;
    return output;
}

Tensor EmbeddingLayer::backward(const Tensor& gradient) {
    // gradient: [batch, seq_len, embedding_dim]
    size_t batch = last_input_.shape()[0];
    size_t seq_len = last_input_.shape()[1];
    
    embedding_grad_ = Tensor({vocab_size_, embedding_dim_}, 0.0);
    
    for (size_t b = 0; b < batch; ++b) {
        for (size_t t = 0; t < seq_len; ++t) {
            size_t idx = static_cast<size_t>(last_input_.data()[b * seq_len + t]);
            for (size_t d = 0; d < embedding_dim_; ++d) {
                embedding_grad_.data()[idx * embedding_dim_ + d] +=
                    gradient.data()[b * (seq_len * embedding_dim_) + t * embedding_dim_ + d];
            }
        }
    }
    return Tensor(last_input_.shape(), 0.0); // No meaningful gradient for indices
}

void EmbeddingLayer::update_parameters(double learning_rate) {
    double batch_size = static_cast<double>(last_input_.shape()[0]);
    for (size_t i = 0; i < embeddings_.size(); ++i)
        embeddings_.data()[i] -= learning_rate * embedding_grad_.data()[i] / batch_size;
}

// ============================================================================
// Transformer Layers
// ============================================================================

// ScaledDotProductAttention
Tensor ScaledDotProductAttention::forward(const Tensor& Q, const Tensor& K, const Tensor& V,
                                           const Tensor* mask) {
    Q_ = Q; K_ = K; V_ = V;
    size_t d_k = Q.shape().back();
    scale_ = std::sqrt(static_cast<double>(d_k));
    
    // scores = Q * K^T / sqrt(d_k)
    // For simplicity, treat as 2D: [seq_len_q, d_k] x [d_k, seq_len_k] = [seq_len_q, seq_len_k]
    Tensor scores = Q.matmul(K.transpose());
    for (size_t i = 0; i < scores.size(); ++i)
        scores.data()[i] /= scale_;
    
    // Apply mask if provided (set masked positions to large negative)
    if (mask) {
        for (size_t i = 0; i < scores.size(); ++i) {
            if (mask->data()[i] == 0.0)
                scores.data()[i] = -1e9;
        }
    }
    
    // Softmax over last dimension
    size_t rows = scores.shape()[0];
    size_t cols = scores.shape()[1];
    attention_weights_ = Tensor(scores.shape(), 0.0);
    for (size_t i = 0; i < rows; ++i) {
        double max_val = -std::numeric_limits<double>::infinity();
        for (size_t j = 0; j < cols; ++j)
            max_val = std::max(max_val, scores.data()[i * cols + j]);
        double sum = 0.0;
        for (size_t j = 0; j < cols; ++j) {
            attention_weights_.data()[i * cols + j] = std::exp(scores.data()[i * cols + j] - max_val);
            sum += attention_weights_.data()[i * cols + j];
        }
        for (size_t j = 0; j < cols; ++j)
            attention_weights_.data()[i * cols + j] /= sum;
    }
    
    // output = attention_weights * V
    return attention_weights_.matmul(V);
}

std::vector<Tensor> ScaledDotProductAttention::backward(const Tensor& gradient) {
    // d_attn_weights = gradient * V^T
    Tensor d_weights = gradient.matmul(V_.transpose());
    
    // Softmax backward
    size_t rows = attention_weights_.shape()[0];
    size_t cols = attention_weights_.shape()[1];
    Tensor d_scores(attention_weights_.shape(), 0.0);
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            double s_ij = attention_weights_.data()[i * cols + j];
            double sum = 0.0;
            for (size_t k = 0; k < cols; ++k)
                sum += d_weights.data()[i * cols + k] * attention_weights_.data()[i * cols + k];
            d_scores.data()[i * cols + j] = s_ij * (d_weights.data()[i * cols + j] - sum);
        }
    }
    
    for (size_t i = 0; i < d_scores.size(); ++i)
        d_scores.data()[i] /= scale_;
    
    // dQ = d_scores * K
    Tensor dQ = d_scores.matmul(K_);
    // dK = d_scores^T * Q
    Tensor dK = d_scores.transpose().matmul(Q_);
    // dV = attention_weights^T * gradient
    Tensor dV = attention_weights_.transpose().matmul(gradient);
    
    return {dQ, dK, dV};
}

// MultiHeadAttentionLayer
MultiHeadAttentionLayer::MultiHeadAttentionLayer(size_t d_model, size_t num_heads)
    : d_model_(d_model), num_heads_(num_heads), d_k_(d_model / num_heads) {
    double limit = std::sqrt(6.0 / (d_model + d_model));
    
    W_q_ = Tensor({d_model, d_model}); W_q_.randomize(-limit, limit);
    W_k_ = Tensor({d_model, d_model}); W_k_.randomize(-limit, limit);
    W_v_ = Tensor({d_model, d_model}); W_v_.randomize(-limit, limit);
    W_o_ = Tensor({d_model, d_model}); W_o_.randomize(-limit, limit);
    
    b_q_ = Tensor({d_model}, 0.0);
    b_k_ = Tensor({d_model}, 0.0);
    b_v_ = Tensor({d_model}, 0.0);
    b_o_ = Tensor({d_model}, 0.0);
    
    W_q_grad_ = Tensor({d_model, d_model}, 0.0);
    W_k_grad_ = Tensor({d_model, d_model}, 0.0);
    W_v_grad_ = Tensor({d_model, d_model}, 0.0);
    W_o_grad_ = Tensor({d_model, d_model}, 0.0);
    b_q_grad_ = Tensor({d_model}, 0.0);
    b_k_grad_ = Tensor({d_model}, 0.0);
    b_v_grad_ = Tensor({d_model}, 0.0);
    b_o_grad_ = Tensor({d_model}, 0.0);
}

Tensor MultiHeadAttentionLayer::forward(const Tensor& input) {
    return forward(input, input, input, nullptr);
}

Tensor MultiHeadAttentionLayer::forward(const Tensor& query, const Tensor& key, const Tensor& value,
                                         const Tensor* mask) {
    query_ = query; key_ = key; value_ = value;
    
    // Project: Q = query * W_q + b_q, etc.
    // For simplicity, treat as 2D [seq_len, d_model]
    Q_proj_ = query.matmul(W_q_);
    K_proj_ = key.matmul(W_k_);
    V_proj_ = value.matmul(W_v_);
    
    // Add biases
    size_t rows_q = Q_proj_.shape()[0];
    size_t rows_k = K_proj_.shape()[0];
    for (size_t i = 0; i < rows_q; ++i)
        for (size_t j = 0; j < d_model_; ++j)
            Q_proj_.data()[i * d_model_ + j] += b_q_.data()[j];
    for (size_t i = 0; i < rows_k; ++i)
        for (size_t j = 0; j < d_model_; ++j) {
            K_proj_.data()[i * d_model_ + j] += b_k_.data()[j];
            V_proj_.data()[i * d_model_ + j] += b_v_.data()[j];
        }
    
    // Simplified multi-head: run attention on full d_model
    // (A production implementation would split into heads, attend, then concat)
    attention_output_ = attention_.forward(Q_proj_, K_proj_, V_proj_, mask);
    
    // Output projection
    Tensor output = attention_output_.matmul(W_o_);
    size_t rows = output.shape()[0];
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < d_model_; ++j)
            output.data()[i * d_model_ + j] += b_o_.data()[j];
    
    last_output_ = output;
    return output;
}

Tensor MultiHeadAttentionLayer::backward(const Tensor& gradient) {
    size_t rows = gradient.shape()[0];
    
    // Output projection backward
    W_o_grad_ = attention_output_.transpose().matmul(gradient);
    b_o_grad_ = Tensor({d_model_}, 0.0);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < d_model_; ++j)
            b_o_grad_.data()[j] += gradient.data()[i * d_model_ + j];
    
    Tensor d_attn = gradient.matmul(W_o_.transpose());
    
    // Attention backward
    std::vector<Tensor> dqkv = attention_.backward(d_attn);
    Tensor& dQ = dqkv[0];
    Tensor& dK = dqkv[1];
    Tensor& dV = dqkv[2];
    
    // Projection backward
    W_q_grad_ = query_.transpose().matmul(dQ);
    W_k_grad_ = key_.transpose().matmul(dK);
    W_v_grad_ = value_.transpose().matmul(dV);
    
    b_q_grad_ = Tensor({d_model_}, 0.0);
    b_k_grad_ = Tensor({d_model_}, 0.0);
    b_v_grad_ = Tensor({d_model_}, 0.0);
    
    size_t rows_q = dQ.shape()[0];
    size_t rows_k = dK.shape()[0];
    for (size_t i = 0; i < rows_q; ++i)
        for (size_t j = 0; j < d_model_; ++j)
            b_q_grad_.data()[j] += dQ.data()[i * d_model_ + j];
    for (size_t i = 0; i < rows_k; ++i)
        for (size_t j = 0; j < d_model_; ++j) {
            b_k_grad_.data()[j] += dK.data()[i * d_model_ + j];
            b_v_grad_.data()[j] += dV.data()[i * d_model_ + j];
        }
    
    // Return gradient w.r.t. query input (for self-attention, all three are the same)
    Tensor d_query = dQ.matmul(W_q_.transpose());
    Tensor d_key = dK.matmul(W_k_.transpose());
    Tensor d_value = dV.matmul(W_v_.transpose());
    
    // Sum if self-attention (query == key == value)
    Tensor result = d_query;
    for (size_t i = 0; i < result.size(); ++i)
        result.data()[i] += d_key.data()[i] + d_value.data()[i];
    
    return result;
}

void MultiHeadAttentionLayer::update_parameters(double learning_rate) {
    auto update = [&](Tensor& W, Tensor& grad, Tensor& b, Tensor& b_grad) {
        for (size_t i = 0; i < W.size(); ++i)
            W.data()[i] -= learning_rate * grad.data()[i];
        for (size_t i = 0; i < b.size(); ++i)
            b.data()[i] -= learning_rate * b_grad.data()[i];
    };
    update(W_q_, W_q_grad_, b_q_, b_q_grad_);
    update(W_k_, W_k_grad_, b_k_, b_k_grad_);
    update(W_v_, W_v_grad_, b_v_, b_v_grad_);
    update(W_o_, W_o_grad_, b_o_, b_o_grad_);
}

// FeedForwardLayer
FeedForwardLayer::FeedForwardLayer(size_t d_model, size_t d_ff)
    : linear1_(d_model, d_ff), linear2_(d_ff, d_model) {}

Tensor FeedForwardLayer::forward(const Tensor& input) {
    last_input_ = input;
    Tensor hidden = linear1_.forward(input);
    hidden = relu_.forward(hidden);
    Tensor output = linear2_.forward(hidden);
    last_output_ = output;
    return output;
}

Tensor FeedForwardLayer::backward(const Tensor& gradient) {
    Tensor d = linear2_.backward(gradient);
    d = relu_.backward(d);
    return linear1_.backward(d);
}

void FeedForwardLayer::update_parameters(double learning_rate) {
    linear1_.update_parameters(learning_rate);
    linear2_.update_parameters(learning_rate);
}

// PositionalEncodingLayer
PositionalEncodingLayer::PositionalEncodingLayer(size_t d_model, size_t max_seq_len, double dropout_rate)
    : d_model_(d_model), max_seq_len_(max_seq_len), dropout_(dropout_rate) {
    // Precompute sinusoidal encoding
    encoding_ = Tensor({max_seq_len, d_model}, 0.0);
    for (size_t pos = 0; pos < max_seq_len; ++pos) {
        for (size_t i = 0; i < d_model; i += 2) {
            double angle = static_cast<double>(pos) / std::pow(10000.0, static_cast<double>(i) / d_model);
            encoding_.data()[pos * d_model + i] = std::sin(angle);
            if (i + 1 < d_model)
                encoding_.data()[pos * d_model + i + 1] = std::cos(angle);
        }
    }
}

Tensor PositionalEncodingLayer::forward(const Tensor& input) {
    // Input: [seq_len, d_model] or [batch, seq_len, d_model]
    last_input_ = input;
    Tensor output = input.clone();
    
    size_t seq_len, d_model;
    if (input.ndim() == 2) {
        seq_len = input.shape()[0];
        d_model = input.shape()[1];
        for (size_t i = 0; i < seq_len; ++i)
            for (size_t j = 0; j < d_model; ++j)
                output.data()[i * d_model + j] += encoding_.data()[i * d_model_ + j];
    } else {
        size_t batch = input.shape()[0];
        seq_len = input.shape()[1];
        d_model = input.shape()[2];
        for (size_t b = 0; b < batch; ++b)
            for (size_t i = 0; i < seq_len; ++i)
                for (size_t j = 0; j < d_model; ++j)
                    output.data()[b * (seq_len * d_model) + i * d_model + j] += encoding_.data()[i * d_model_ + j];
    }
    
    output = dropout_.forward(output);
    last_output_ = output;
    return output;
}

Tensor PositionalEncodingLayer::backward(const Tensor& gradient) {
    return dropout_.backward(gradient);
}

// TransformerEncoderLayer
TransformerEncoderLayer::TransformerEncoderLayer(size_t d_model, size_t num_heads, size_t d_ff,
                                                   double dropout_rate)
    : self_attention_(d_model, num_heads),
      feed_forward_(d_model, d_ff),
      norm1_(d_model),
      norm2_(d_model),
      dropout1_(dropout_rate),
      dropout2_(dropout_rate) {}

Tensor TransformerEncoderLayer::forward(const Tensor& input) {
    last_input_ = input;
    
    // Self-attention with residual + layer norm
    Tensor attn = self_attention_.forward(input);
    attn = dropout1_.forward(attn);
    // Residual connection
    Tensor residual1 = input.clone();
    for (size_t i = 0; i < residual1.size(); ++i)
        residual1.data()[i] += attn.data()[i];
    attn_output_ = norm1_.forward(residual1);
    
    // Feed-forward with residual + layer norm
    ff_input_ = attn_output_;
    Tensor ff = feed_forward_.forward(attn_output_);
    ff = dropout2_.forward(ff);
    Tensor residual2 = attn_output_.clone();
    for (size_t i = 0; i < residual2.size(); ++i)
        residual2.data()[i] += ff.data()[i];
    Tensor output = norm2_.forward(residual2);
    
    last_output_ = output;
    return output;
}

Tensor TransformerEncoderLayer::backward(const Tensor& gradient) {
    // Backward through norm2
    Tensor d = norm2_.backward(gradient);
    
    // Residual split
    Tensor d_ff = d.clone();
    Tensor d_res2 = d.clone();
    
    // Backward through dropout2 + feed_forward
    d_ff = dropout2_.backward(d_ff);
    d_ff = feed_forward_.backward(d_ff);
    
    // Add residual gradient
    for (size_t i = 0; i < d_ff.size(); ++i)
        d_ff.data()[i] += d_res2.data()[i];
    
    // Backward through norm1
    Tensor d_norm1 = norm1_.backward(d_ff);
    
    // Residual split
    Tensor d_attn = d_norm1.clone();
    Tensor d_res1 = d_norm1.clone();
    
    // Backward through dropout1 + self_attention
    d_attn = dropout1_.backward(d_attn);
    d_attn = self_attention_.backward(d_attn);
    
    // Add residual gradient
    for (size_t i = 0; i < d_attn.size(); ++i)
        d_attn.data()[i] += d_res1.data()[i];
    
    return d_attn;
}

void TransformerEncoderLayer::update_parameters(double learning_rate) {
    self_attention_.update_parameters(learning_rate);
    feed_forward_.update_parameters(learning_rate);
    norm1_.update_parameters(learning_rate);
    norm2_.update_parameters(learning_rate);
}

// TransformerDecoderLayer
TransformerDecoderLayer::TransformerDecoderLayer(size_t d_model, size_t num_heads, size_t d_ff,
                                                   double dropout_rate)
    : self_attention_(d_model, num_heads),
      cross_attention_(d_model, num_heads),
      feed_forward_(d_model, d_ff),
      norm1_(d_model),
      norm2_(d_model),
      norm3_(d_model),
      dropout1_(dropout_rate),
      dropout2_(dropout_rate),
      dropout3_(dropout_rate) {}

Tensor TransformerDecoderLayer::forward(const Tensor& input) {
    return forward(input, encoder_output_, nullptr, nullptr);
}

Tensor TransformerDecoderLayer::forward(const Tensor& target, const Tensor& encoder_output,
                                         const Tensor* target_mask, const Tensor* memory_mask) {
    last_input_ = target;
    encoder_output_ = encoder_output;
    
    // Self-attention on target with residual + layer norm
    Tensor self_attn = self_attention_.forward(target, target, target, target_mask);
    self_attn = dropout1_.forward(self_attn);
    Tensor residual1 = target.clone();
    for (size_t i = 0; i < residual1.size(); ++i)
        residual1.data()[i] += self_attn.data()[i];
    self_attn_output_ = norm1_.forward(residual1);
    
    // Cross-attention with encoder output
    Tensor cross_attn = cross_attention_.forward(self_attn_output_, encoder_output, encoder_output, memory_mask);
    cross_attn = dropout2_.forward(cross_attn);
    Tensor residual2 = self_attn_output_.clone();
    for (size_t i = 0; i < residual2.size(); ++i)
        residual2.data()[i] += cross_attn.data()[i];
    cross_attn_output_ = norm2_.forward(residual2);
    
    // Feed-forward with residual + layer norm
    ff_input_ = cross_attn_output_;
    Tensor ff = feed_forward_.forward(cross_attn_output_);
    ff = dropout3_.forward(ff);
    Tensor residual3 = cross_attn_output_.clone();
    for (size_t i = 0; i < residual3.size(); ++i)
        residual3.data()[i] += ff.data()[i];
    Tensor output = norm3_.forward(residual3);
    
    last_output_ = output;
    return output;
}

Tensor TransformerDecoderLayer::backward(const Tensor& gradient) {
    // Backward through norm3
    Tensor d = norm3_.backward(gradient);
    Tensor d_ff = d.clone();
    Tensor d_res3 = d.clone();
    
    d_ff = dropout3_.backward(d_ff);
    d_ff = feed_forward_.backward(d_ff);
    for (size_t i = 0; i < d_ff.size(); ++i)
        d_ff.data()[i] += d_res3.data()[i];
    
    // Backward through norm2
    Tensor d_norm2 = norm2_.backward(d_ff);
    Tensor d_cross = d_norm2.clone();
    Tensor d_res2 = d_norm2.clone();
    
    d_cross = dropout2_.backward(d_cross);
    d_cross = cross_attention_.backward(d_cross);
    for (size_t i = 0; i < d_cross.size(); ++i)
        d_cross.data()[i] += d_res2.data()[i];
    
    // Backward through norm1
    Tensor d_norm1 = norm1_.backward(d_cross);
    Tensor d_self = d_norm1.clone();
    Tensor d_res1 = d_norm1.clone();
    
    d_self = dropout1_.backward(d_self);
    d_self = self_attention_.backward(d_self);
    for (size_t i = 0; i < d_self.size(); ++i)
        d_self.data()[i] += d_res1.data()[i];
    
    return d_self;
}

void TransformerDecoderLayer::update_parameters(double learning_rate) {
    self_attention_.update_parameters(learning_rate);
    cross_attention_.update_parameters(learning_rate);
    feed_forward_.update_parameters(learning_rate);
    norm1_.update_parameters(learning_rate);
    norm2_.update_parameters(learning_rate);
    norm3_.update_parameters(learning_rate);
}

} // namespace deep_learning
} // namespace ml
