#include "deep_learning/layers/avg_pool_2d_layer.h"

namespace ml {
namespace deep_learning {

AvgPool2DLayer::AvgPool2DLayer(int kernel_size, int stride)
    : kernel_size_(kernel_size), stride_(stride) {}

Tensor AvgPool2DLayer::forward(const Tensor& input) {
    // Dummy implementation
    return input;
}

Tensor AvgPool2DLayer::backward(const Tensor& gradient) {
    // Dummy implementation
    return gradient;
}

} // namespace deep_learning
} // namespace ml
