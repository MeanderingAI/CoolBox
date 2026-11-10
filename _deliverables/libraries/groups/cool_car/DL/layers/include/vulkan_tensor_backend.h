#ifndef COOLBOX_VULKAN_TENSOR_BACKEND_H
#define COOLBOX_VULKAN_TENSOR_BACKEND_H

#include "tensor_backend.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace ml {
namespace deep_learning {
namespace detail {

struct VulkanTensorStorage;
using VulkanTensorStoragePtr =
    std::shared_ptr<VulkanTensorStorage>;

std::vector<double> vulkan_tensor_execute(
    VulkanTensorOperation operation,
    const std::vector<double>& left,
    const std::vector<double>& right,
    std::size_t output_size,
    std::size_t rows,
    std::size_t columns,
    std::size_t inner,
    double scalar = 0.0);

VulkanTensorStoragePtr vulkan_tensor_upload(
    const std::vector<double>& values);
VulkanTensorStoragePtr vulkan_tensor_execute_resident(
    VulkanTensorOperation operation,
    const VulkanTensorStoragePtr& left,
    const VulkanTensorStoragePtr& right,
    std::size_t output_size,
    std::size_t rows,
    std::size_t columns,
    std::size_t inner,
    double scalar = 0.0,
    const VulkanTensorStoragePtr& auxiliary = {});
std::vector<double> vulkan_tensor_download(
    const VulkanTensorStoragePtr& storage);
bool vulkan_tensor_storage_matches_precision(
    const VulkanTensorStoragePtr& storage);
void vulkan_tensor_adamw_step_in_place(
    const VulkanTensorStoragePtr& values,
    const VulkanTensorStoragePtr& gradients,
    const VulkanTensorStoragePtr& first_moment,
    const VulkanTensorStoragePtr& second_moment,
    double learning_rate,
    double beta1,
    double beta2,
    double beta1_correction,
    double beta2_correction,
    double epsilon,
    double weight_decay,
    double gradient_scale);

} // namespace detail
} // namespace deep_learning
} // namespace ml

#endif
