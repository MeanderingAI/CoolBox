#ifndef COMPUTER_VISION_LAYERS_H
#define COMPUTER_VISION_LAYERS_H

#include "../deep_learning/tensor.h"
#include "../deep_learning/layer.h"
#include "image.h"
#include <memory>

namespace ml {
namespace cv {

// Forward declare namespace
namespace dl = deep_learning;

// Convert Image to Tensor for neural network processing
dl::Tensor image_to_tensor(const Image& image);

// Convert Tensor back to Image
Image tensor_to_image(const dl::Tensor& tensor, ImageFormat format = ImageFormat::RGB);

// 2D Convolution Layer
class Conv2DLayer : public dl::Layer {
public:
    Conv2DLayer(int in_channels, int out_channels, int kernel_size, 
                int stride = 1, int padding = 0);
    ~Conv2DLayer() override = default;

    dl::Tensor forward(const dl::Tensor& input) override;
    dl::Tensor backward(const dl::Tensor& grad_output) override;
    
    std::string name() const override { return "Conv2D"; }
    bool has_parameters() const override { return true; }
    
    int in_channels() const { return in_channels_; }
    int out_channels() const { return out_channels_; }
    int kernel_size() const { return kernel_size_; }
    int stride() const { return stride_; }
    int padding() const { return padding_; }

private:
    int in_channels_;
    int out_channels_;
    int kernel_size_;
    int stride_;
    int padding_;
    
    dl::Tensor weights_;
    dl::Tensor bias_;
    dl::Tensor weights_grad_;
    dl::Tensor bias_grad_;
    dl::Tensor input_cache_;
    
    void initialize_weights();
    dl::Tensor im2col(const dl::Tensor& input) const;
    dl::Tensor col2im(const dl::Tensor& col, int height, int width) const;
};

// 2D Max Pooling Layer
class MaxPool2DLayer : public dl::Layer {
public:
    MaxPool2DLayer(int kernel_size, int stride = -1);  // stride=-1 means stride=kernel_size
    ~MaxPool2DLayer() override = default;

    dl::Tensor forward(const dl::Tensor& input) override;
    dl::Tensor backward(const dl::Tensor& grad_output) override;
    
    std::string name() const override { return "MaxPool2D"; }
    
    int kernel_size() const { return kernel_size_; }
    int stride() const { return stride_; }

private:
    int kernel_size_;
    int stride_;
    
    dl::Tensor input_cache_;
    std::vector<int> max_indices_;
};

// 2D Average Pooling Layer
class AvgPool2DLayer : public dl::Layer {
public:
    AvgPool2DLayer(int kernel_size, int stride = -1);
    ~AvgPool2DLayer() override = default;

    dl::Tensor forward(const dl::Tensor& input) override;
    dl::Tensor backward(const dl::Tensor& grad_output) override;
    
    std::string name() const override { return "AvgPool2D"; }
    
    int kernel_size() const { return kernel_size_; }
    int stride() const { return stride_; }

private:
    int kernel_size_;
    int stride_;
    
    dl::Tensor input_cache_;
};

// Batch Normalization 2D (for convolutional layers)
class BatchNorm2DLayer : public dl::Layer {
public:
    BatchNorm2DLayer(int num_features, float eps = 1e-5f, float momentum = 0.1f);
    ~BatchNorm2DLayer() override = default;

    dl::Tensor forward(const dl::Tensor& input) override;
    dl::Tensor backward(const dl::Tensor& grad_output) override;
    
    std::string name() const override { return "BatchNorm2D"; }
    bool has_parameters() const override { return true; }
    
    void set_training(bool training) { training_ = training; }
    bool is_training() const { return training_; }

private:
    int num_features_;
    float eps_;
    float momentum_;
    bool training_;
    
    dl::Tensor gamma_;  // Scale parameter
    dl::Tensor beta_;   // Shift parameter
    dl::Tensor gamma_grad_;
    dl::Tensor beta_grad_;
    
    dl::Tensor running_mean_;
    dl::Tensor running_var_;
    
    dl::Tensor input_cache_;
    dl::Tensor normalized_cache_;
};

// Flatten layer - converts 4D tensor (batch, channels, height, width) to 2D (batch, features)
class FlattenLayer : public dl::Layer {
public:
    FlattenLayer() = default;
    ~FlattenLayer() override = default;

    dl::Tensor forward(const dl::Tensor& input) override;
    dl::Tensor backward(const dl::Tensor& grad_output) override;
    
    std::string name() const override { return "Flatten"; }

private:
    std::vector<size_t> input_shape_;
};

// Global Average Pooling - reduces spatial dimensions to 1x1
class GlobalAvgPool2DLayer : public dl::Layer {
public:
    GlobalAvgPool2DLayer() = default;
    ~GlobalAvgPool2DLayer() override = default;

    dl::Tensor forward(const dl::Tensor& input) override;
    dl::Tensor backward(const dl::Tensor& grad_output) override;
    
    std::string name() const override { return "GlobalAvgPool2D"; }

private:
    dl::Tensor input_cache_;
};

} // namespace cv
} // namespace ml

#endif // COMPUTER_VISION_LAYERS_H
