#ifndef COOLBOX_VULKAN_TENSOR_BACKEND_H
#define COOLBOX_VULKAN_TENSOR_BACKEND_H

#include "tensor_backend.h"

#include <cstddef>
#include <vector>

namespace ml {
namespace deep_learning {
namespace detail {

std::vector<double> vulkan_tensor_execute(
    VulkanTensorOperation operation,
    const std::vector<double>& left,
    const std::vector<double>& right,
    std::size_t output_size,
    std::size_t rows,
    std::size_t columns,
    std::size_t inner,
    double scalar = 0.0);

} // namespace detail
} // namespace deep_learning
} // namespace ml

#endif
