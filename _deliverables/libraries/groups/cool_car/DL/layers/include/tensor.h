#ifndef TENSOR_H
#define TENSOR_H

#include <complex>
#include <type_traits>
#include <vector>
#include <memory>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>
#include "tensor_backend.h"
#ifdef COOLBOX_HAS_VULKAN_TENSOR
#include "vulkan_tensor_backend.h"
#endif
namespace ml {
namespace deep_learning {

class Tensor {
    // Internal utility: convert N-D indices to flat index
    size_t flat_index(const std::vector<size_t>& indices) const;
public:
    // Constructors
    Tensor() : data_(), shape_() {}
    
    Tensor(const std::vector<size_t>& shape);
    Tensor(const std::vector<size_t>& shape, double fill_value);
    Tensor(const std::vector<size_t>& shape, const std::vector<double>& data);
    
    // Getters
    const std::vector<double>& data() const;
    std::vector<double>& data();
    const std::vector<size_t>& shape() const { return shape_; }
    size_t size() const { return data_.size(); }
    size_t ndim() const { return shape_.size(); }
    
    // Element access
    double& operator()(const std::vector<size_t>& indices);
    const double& operator()(const std::vector<size_t>& indices) const;
    double& at(size_t index) { return data().at(index); }
    const double& at(size_t index) const { return data().at(index); }
    
    // Operations
    Tensor reshape(const std::vector<size_t>& new_shape) const;
    Tensor transpose() const; // For 2D tensors
    
    // Math operations
    Tensor operator+(const Tensor& other) const;
    Tensor operator-(const Tensor& other) const;
    Tensor operator*(const Tensor& other) const; // Element-wise
    Tensor operator/(const Tensor& other) const; // Element-wise
    
    Tensor& operator+=(const Tensor& other);
    Tensor& operator-=(const Tensor& other);
    
    // Scalar operations
    Tensor operator*(double scalar) const;
    Tensor operator/(double scalar) const;
    
    // Matrix multiplication
    Tensor matmul(const Tensor& other) const;
    Tensor matmul_bias(
        const Tensor& weights,
        const Tensor& bias) const;
    Tensor relu() const;
    Tensor relu_backward(const Tensor& gradient) const;
    Tensor row_softmax() const;
    Tensor masked_fill(
        const Tensor& mask,
        double fill_value) const;
    Tensor layer_norm(
        const Tensor& gamma,
        const Tensor& beta,
        double epsilon) const;
    Tensor layer_norm_backward(
        const Tensor& gradient,
        const Tensor& gamma,
        double epsilon) const;
    Tensor layer_norm_gamma_gradient(
        const Tensor& gradient,
        double epsilon) const;
    Tensor row_sum() const;
    Tensor broadcast_add(const Tensor& row) const;
    Tensor broadcast_multiply(const Tensor& row) const;
    Tensor add_prefix(const Tensor& prefix) const;
    Tensor slice_columns(size_t offset, size_t width) const;
    Tensor concat_columns(const Tensor& other) const;
    Tensor squared_sum_tensor() const;
    double squared_sum() const;
    Tensor adamw_update(
        const Tensor& first_moment,
        const Tensor& second_moment,
        double learning_rate,
        double beta1_correction,
        double beta2_correction,
        double epsilon) const;
    
    // Utility
    void fill(double value);
    void randomize(double min = -1.0, double max = 1.0);
    Tensor clone() const;

    // Reduction
    double sum() const;
    double mean() const;

private:
    mutable std::vector<double> data_;
    std::vector<size_t> shape_;
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    mutable detail::VulkanTensorStoragePtr device_storage_;
    mutable bool host_valid_ = true;

    void ensure_host() const;
    void ensure_device() const;
    Tensor device_result(
        const std::vector<size_t>& shape,
        detail::VulkanTensorStoragePtr storage) const;
#endif
    
    size_t compute_index(const std::vector<size_t>& indices) const;
    bool is_shape_compatible(const Tensor& other) const;
};

} // namespace deep_learning
} // namespace ml

#endif // TENSOR_H
