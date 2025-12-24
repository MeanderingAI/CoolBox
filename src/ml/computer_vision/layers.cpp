#include "ml/computer_vision/layers.h"
#include <cmath>
#include <stdexcept>
#include <random>
#include <limits>

namespace ml {
namespace cv {

// Image to Tensor conversion
dl::Tensor image_to_tensor(const Image& image) {
    // Convert HWC (Height, Width, Channels) to CHW (Channels, Height, Width)
    std::vector<size_t> shape = {1, static_cast<size_t>(image.channels()), 
                                  static_cast<size_t>(image.height()), 
                                  static_cast<size_t>(image.width())};
    dl::Tensor tensor(shape);
    
    auto& data = tensor.data();
    const auto& img_data = image.data();
    
    for (int c = 0; c < image.channels(); ++c) {
        for (int h = 0; h < image.height(); ++h) {
            for (int w = 0; w < image.width(); ++w) {
                int tensor_idx = c * image.height() * image.width() + h * image.width() + w;
                int img_idx = (h * image.width() + w) * image.channels() + c;
                data[tensor_idx] = img_data[img_idx];
            }
        }
    }
    
    return tensor;
}

// Tensor to Image conversion
Image tensor_to_image(const dl::Tensor& tensor, ImageFormat format) {
    const auto& shape = tensor.shape();
    
    // Expect shape [1, C, H, W] or [C, H, W]
    int c, h, w;
    if (shape.size() == 4) {
        c = shape[1];
        h = shape[2];
        w = shape[3];
    } else if (shape.size() == 3) {
        c = shape[0];
        h = shape[1];
        w = shape[2];
    } else {
        throw std::invalid_argument("Tensor must have 3 or 4 dimensions");
    }
    
    Image image(h, w, c, std::vector<float>(h * w * c));
    
    const auto& tensor_data = tensor.data();
    auto& img_data = image.data();
    
    for (int ci = 0; ci < c; ++ci) {
        for (int hi = 0; hi < h; ++hi) {
            for (int wi = 0; wi < w; ++wi) {
                int tensor_idx = ci * h * w + hi * w + wi;
                int img_idx = (hi * w + wi) * c + ci;
                img_data[img_idx] = tensor_data[tensor_idx];
            }
        }
    }
    
    return image;
}

// Conv2DLayer implementation
Conv2DLayer::Conv2DLayer(int in_channels, int out_channels, int kernel_size, 
                         int stride, int padding)
    : in_channels_(in_channels), out_channels_(out_channels),
      kernel_size_(kernel_size), stride_(stride), padding_(padding) {
    initialize_weights();
}

void Conv2DLayer::initialize_weights() {
    // Weights shape: [out_channels, in_channels, kernel_size, kernel_size]
    std::vector<size_t> weight_shape = {static_cast<size_t>(out_channels_), 
                                          static_cast<size_t>(in_channels_), 
                                          static_cast<size_t>(kernel_size_), 
                                          static_cast<size_t>(kernel_size_)};
    weights_ = dl::Tensor(weight_shape);
    weights_grad_ = dl::Tensor(weight_shape);
    
    // Bias shape: [out_channels]
    bias_ = dl::Tensor({static_cast<size_t>(out_channels_)});
    bias_grad_ = dl::Tensor({static_cast<size_t>(out_channels_)});
    
    // Xavier initialization
    float std = std::sqrt(2.0f / (in_channels_ * kernel_size_ * kernel_size_));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, std);
    
    for (auto& w : weights_.data()) {
        w = dist(gen);
    }
    
    // Initialize bias to zero
    for (auto& b : bias_.data()) {
        b = 0.0f;
    }
}

dl::Tensor Conv2DLayer::forward(const dl::Tensor& input) {
    input_cache_ = input;
    
    const auto& shape = input.shape();
    int batch = shape[0];
    int in_h = shape[2];
    int in_w = shape[3];
    
    // Calculate output dimensions
    int out_h = (in_h + 2 * padding_ - kernel_size_) / stride_ + 1;
    int out_w = (in_w + 2 * padding_ - kernel_size_) / stride_ + 1;
    
    dl::Tensor output({static_cast<size_t>(batch), static_cast<size_t>(out_channels_), 
                       static_cast<size_t>(out_h), static_cast<size_t>(out_w)});
    auto& out_data = output.data();
    const auto& in_data = input.data();
    const auto& w_data = weights_.data();
    const auto& b_data = bias_.data();
    
    // Simplified convolution (not using im2col for clarity)
    for (int b = 0; b < batch; ++b) {
        for (int oc = 0; oc < out_channels_; ++oc) {
            for (int oh = 0; oh < out_h; ++oh) {
                for (int ow = 0; ow < out_w; ++ow) {
                    float sum = b_data[oc];
                    
                    for (int ic = 0; ic < in_channels_; ++ic) {
                        for (int kh = 0; kh < kernel_size_; ++kh) {
                            for (int kw = 0; kw < kernel_size_; ++kw) {
                                int ih = oh * stride_ - padding_ + kh;
                                int iw = ow * stride_ - padding_ + kw;
                                
                                if (ih >= 0 && ih < in_h && iw >= 0 && iw < in_w) {
                                    int in_idx = ((b * in_channels_ + ic) * in_h + ih) * in_w + iw;
                                    int w_idx = ((oc * in_channels_ + ic) * kernel_size_ + kh) * kernel_size_ + kw;
                                    sum += in_data[in_idx] * w_data[w_idx];
                                }
                            }
                        }
                    }
                    
                    int out_idx = ((b * out_channels_ + oc) * out_h + oh) * out_w + ow;
                    out_data[out_idx] = sum;
                }
            }
        }
    }
    
    return output;
}

dl::Tensor Conv2DLayer::backward(const dl::Tensor& grad_output) {
    // Simplified backward pass
    const auto& in_shape = input_cache_.shape();
    dl::Tensor grad_input(in_shape);
    
    // This is a simplified implementation
    // Full implementation would compute proper gradients for weights and input
    
    return grad_input;
}

dl::Tensor Conv2DLayer::im2col(const dl::Tensor& input) const {
    // im2col implementation stub
    return input;
}

dl::Tensor Conv2DLayer::col2im(const dl::Tensor& col, int height, int width) const {
    // col2im implementation stub
    return col;
}

// MaxPool2DLayer implementation
MaxPool2DLayer::MaxPool2DLayer(int kernel_size, int stride)
    : kernel_size_(kernel_size), stride_(stride == -1 ? kernel_size : stride) {}

dl::Tensor MaxPool2DLayer::forward(const dl::Tensor& input) {
    input_cache_ = input;
    
    const auto& shape = input.shape();
    int batch = shape[0];
    int channels = shape[1];
    int in_h = shape[2];
    int in_w = shape[3];
    
    int out_h = (in_h - kernel_size_) / stride_ + 1;
    int out_w = (in_w - kernel_size_) / stride_ + 1;
    
    dl::Tensor output({static_cast<size_t>(batch), static_cast<size_t>(channels), 
                       static_cast<size_t>(out_h), static_cast<size_t>(out_w)});
    auto& out_data = output.data();
    const auto& in_data = input.data();
    
    max_indices_.resize(batch * channels * out_h * out_w);
    
    for (int b = 0; b < batch; ++b) {
        for (int c = 0; c < channels; ++c) {
            for (int oh = 0; oh < out_h; ++oh) {
                for (int ow = 0; ow < out_w; ++ow) {
                    float max_val = -std::numeric_limits<float>::infinity();
                    int max_idx = 0;
                    
                    for (int kh = 0; kh < kernel_size_; ++kh) {
                        for (int kw = 0; kw < kernel_size_; ++kw) {
                            int ih = oh * stride_ + kh;
                            int iw = ow * stride_ + kw;
                            int in_idx = ((b * channels + c) * in_h + ih) * in_w + iw;
                            
                            if (in_data[in_idx] > max_val) {
                                max_val = in_data[in_idx];
                                max_idx = in_idx;
                            }
                        }
                    }
                    
                    int out_idx = ((b * channels + c) * out_h + oh) * out_w + ow;
                    out_data[out_idx] = max_val;
                    max_indices_[out_idx] = max_idx;
                }
            }
        }
    }
    
    return output;
}

dl::Tensor MaxPool2DLayer::backward(const dl::Tensor& grad_output) {
    dl::Tensor grad_input(input_cache_.shape());
    auto& grad_in = grad_input.data();
    const auto& grad_out = grad_output.data();
    
    // Initialize to zero
    std::fill(grad_in.begin(), grad_in.end(), 0.0f);
    
    // Route gradients to max locations
    for (size_t i = 0; i < max_indices_.size(); ++i) {
        grad_in[max_indices_[i]] += grad_out[i];
    }
    
    return grad_input;
}

// AvgPool2DLayer implementation
AvgPool2DLayer::AvgPool2DLayer(int kernel_size, int stride)
    : kernel_size_(kernel_size), stride_(stride == -1 ? kernel_size : stride) {}

dl::Tensor AvgPool2DLayer::forward(const dl::Tensor& input) {
    input_cache_ = input;
    
    const auto& shape = input.shape();
    int batch = shape[0];
    int channels = shape[1];
    int in_h = shape[2];
    int in_w = shape[3];
    
    int out_h = (in_h - kernel_size_) / stride_ + 1;
    int out_w = (in_w - kernel_size_) / stride_ + 1;
    
    dl::Tensor output({static_cast<size_t>(batch), static_cast<size_t>(channels), 
                       static_cast<size_t>(out_h), static_cast<size_t>(out_w)});
    auto& out_data = output.data();
    const auto& in_data = input.data();
    
    float scale = 1.0f / (kernel_size_ * kernel_size_);
    
    for (int b = 0; b < batch; ++b) {
        for (int c = 0; c < channels; ++c) {
            for (int oh = 0; oh < out_h; ++oh) {
                for (int ow = 0; ow < out_w; ++ow) {
                    float sum = 0.0f;
                    
                    for (int kh = 0; kh < kernel_size_; ++kh) {
                        for (int kw = 0; kw < kernel_size_; ++kw) {
                            int ih = oh * stride_ + kh;
                            int iw = ow * stride_ + kw;
                            int in_idx = ((b * channels + c) * in_h + ih) * in_w + iw;
                            sum += in_data[in_idx];
                        }
                    }
                    
                    int out_idx = ((b * channels + c) * out_h + oh) * out_w + ow;
                    out_data[out_idx] = sum * scale;
                }
            }
        }
    }
    
    return output;
}

dl::Tensor AvgPool2DLayer::backward(const dl::Tensor& grad_output) {
    // Simplified backward pass
    dl::Tensor grad_input(input_cache_.shape());
    return grad_input;
}

// BatchNorm2DLayer implementation
BatchNorm2DLayer::BatchNorm2DLayer(int num_features, float eps, float momentum)
    : num_features_(num_features), eps_(eps), momentum_(momentum), training_(true) {
    
    gamma_ = dl::Tensor({static_cast<size_t>(num_features)});
    beta_ = dl::Tensor({static_cast<size_t>(num_features)});
    gamma_grad_ = dl::Tensor({static_cast<size_t>(num_features)});
    beta_grad_ = dl::Tensor({static_cast<size_t>(num_features)});
    running_mean_ = dl::Tensor({static_cast<size_t>(num_features)});
    running_var_ = dl::Tensor({static_cast<size_t>(num_features)});
    
    // Initialize gamma to 1, others to 0
    std::fill(gamma_.data().begin(), gamma_.data().end(), 1.0f);
    std::fill(beta_.data().begin(), beta_.data().end(), 0.0f);
    std::fill(running_mean_.data().begin(), running_mean_.data().end(), 0.0f);
    std::fill(running_var_.data().begin(), running_var_.data().end(), 1.0f);
}

dl::Tensor BatchNorm2DLayer::forward(const dl::Tensor& input) {
    input_cache_ = input;
    
    // Simplified implementation - full batch norm would compute mean/var per channel
    return input;
}

dl::Tensor BatchNorm2DLayer::backward(const dl::Tensor& grad_output) {
    return grad_output;
}

// FlattenLayer implementation
dl::Tensor FlattenLayer::forward(const dl::Tensor& input) {
    input_shape_ = input.shape();
    
    size_t batch = input_shape_[0];
    size_t total_features = 1;
    for (size_t i = 1; i < input_shape_.size(); ++i) {
        total_features *= input_shape_[i];
    }
    
    dl::Tensor output({batch, total_features});
    output.data() = input.data();
    
    return output;
}

dl::Tensor FlattenLayer::backward(const dl::Tensor& grad_output) {
    dl::Tensor grad_input(input_shape_);
    grad_input.data() = grad_output.data();
    return grad_input;
}

// GlobalAvgPool2DLayer implementation
dl::Tensor GlobalAvgPool2DLayer::forward(const dl::Tensor& input) {
    input_cache_ = input;
    
    const auto& shape = input.shape();
    int batch = shape[0];
    int channels = shape[1];
    int h = shape[2];
    int w = shape[3];
    
    dl::Tensor output({static_cast<size_t>(batch), static_cast<size_t>(channels), 1, 1});
    auto& out_data = output.data();
    const auto& in_data = input.data();
    
    float scale = 1.0f / (h * w);
    
    for (int b = 0; b < batch; ++b) {
        for (int c = 0; c < channels; ++c) {
            float sum = 0.0f;
            for (int i = 0; i < h; ++i) {
                for (int j = 0; j < w; ++j) {
                    int idx = ((b * channels + c) * h + i) * w + j;
                    sum += in_data[idx];
                }
            }
            out_data[b * channels + c] = sum * scale;
        }
    }
    
    return output;
}

dl::Tensor GlobalAvgPool2DLayer::backward(const dl::Tensor& grad_output) {
    // Simplified backward pass
    dl::Tensor grad_input(input_cache_.shape());
    return grad_input;
}

} // namespace cv
} // namespace ml
