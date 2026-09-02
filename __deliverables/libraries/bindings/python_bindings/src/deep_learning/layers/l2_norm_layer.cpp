#include "DL/layers/include/l2_norm_layer.h"
#include <cmath>

namespace ml {
namespace deep_learning {

Tensor L2NormLayer::forward(const Tensor& input) {
    // Dummy implementation: return input
    return input;
}

Tensor L2NormLayer::backward(const Tensor& gradient) {
    // Dummy implementation: return gradient
    return gradient;
}

} // namespace deep_learning
} // namespace ml
