#include "../include/tensor.h"
#ifdef COOLBOX_HAS_VULKAN_TENSOR
#include "../include/vulkan_tensor_backend.h"
#endif
#include <random>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>
namespace ml {
namespace deep_learning {

Tensor::Tensor(const std::vector<size_t>& shape) : shape_(shape) {
    size_t total = 1;
    for (auto s : shape) total *= s;
    data_.resize(total, 0.0);
}

Tensor::Tensor(const std::vector<size_t>& shape, double fill_value) : shape_(shape) {
    size_t total = 1;
    for (auto s : shape) total *= s;
    data_.resize(total, fill_value);
}

Tensor::Tensor(const std::vector<size_t>& shape, const std::vector<double>& data)
    : data_(data), shape_(shape) {}

const std::vector<double>& Tensor::data() const {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    ensure_host();
#endif
    return data_;
}

std::vector<double>& Tensor::data() {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    ensure_host();
    device_storage_.reset();
#endif
    return data_;
}

#ifdef COOLBOX_HAS_VULKAN_TENSOR
void Tensor::ensure_host() const {
    if (!host_valid_) {
        if (!device_storage_) {
            throw std::logic_error(
                "Tensor has neither valid host nor device data");
        }
        data_ = detail::vulkan_tensor_download(device_storage_);
        host_valid_ = true;
    }
}

void Tensor::ensure_device() const {
    if (device_storage_ &&
        detail::vulkan_tensor_storage_matches_precision(
            device_storage_)) {
        return;
    }
    ensure_host();
    device_storage_ = detail::vulkan_tensor_upload(data_);
}

Tensor Tensor::device_result(
    const std::vector<size_t>& shape,
    detail::VulkanTensorStoragePtr storage) const {
    Tensor result(shape);
    result.device_storage_ = std::move(storage);
    result.host_valid_ = false;
    return result;
}
#endif

size_t Tensor::flat_index(const std::vector<size_t>& indices) const {
    if (indices.size() != shape_.size()) {
        throw std::invalid_argument("Index dimensions mismatch");
    }
    size_t idx = 0;
    size_t multiplier = 1;
    for (int i = static_cast<int>(shape_.size()) - 1; i >= 0; --i) {
        idx += indices[i] * multiplier;
        multiplier *= shape_[i];
    }
    return idx;
}

double& Tensor::operator()(const std::vector<size_t>& indices) {
    return data()[flat_index(indices)];
}

const double& Tensor::operator()(const std::vector<size_t>& indices) const {
    return data()[flat_index(indices)];
}

Tensor Tensor::reshape(const std::vector<size_t>& new_shape) const {
    size_t total = 1;
    for (auto s : new_shape) total *= s;
    if (total != data_.size()) throw std::invalid_argument("Reshape size mismatch");
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (device_storage_) {
        Tensor result(new_shape);
        result.data_ = data_;
        result.device_storage_ = device_storage_;
        result.host_valid_ = host_valid_;
        return result;
    }
#endif
    Tensor result(new_shape, data());
    return result;
}

Tensor Tensor::transpose() const {
    if (shape_.size() != 2) throw std::invalid_argument("Transpose only for 2D tensors");
    size_t rows = shape_[0], cols = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        return device_result(
            {cols, rows},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::TRANSPOSE,
                device_storage_, {}, data_.size(), rows, cols, 0));
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    Tensor result({cols, rows});
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            result.data_[j * rows + i] = data_[i * cols + j];
    return result;
}

Tensor Tensor::operator+(const Tensor& other) const {
    if (shape_ != other.shape_)
        throw std::invalid_argument("Tensor shapes must match for addition");
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        other.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::ADD,
                device_storage_, other.device_storage_,
                data_.size(), 0, 0, 0));
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    other.data();
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] + other.data_[i];
    return result;
}

Tensor Tensor::operator-(const Tensor& other) const {
    if (shape_ != other.shape_)
        throw std::invalid_argument("Tensor shapes must match for subtraction");
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        other.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::SUBTRACT,
                device_storage_, other.device_storage_,
                data_.size(), 0, 0, 0));
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    other.data();
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] - other.data_[i];
    return result;
}

Tensor Tensor::operator*(const Tensor& other) const {
    if (shape_ != other.shape_)
        throw std::invalid_argument(
            "Tensor shapes must match for element-wise multiplication");
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        other.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::MULTIPLY,
                device_storage_, other.device_storage_,
                data_.size(), 0, 0, 0));
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    other.data();
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] * other.data_[i];
    return result;
}

Tensor Tensor::operator/(const Tensor& other) const {
    if (shape_ != other.shape_)
        throw std::invalid_argument(
            "Tensor shapes must match for element-wise division");
    for (double value : other.data()) {
        if (std::abs(value) < 1e-10)
            throw std::runtime_error("Division by zero");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        other.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::DIVIDE,
                device_storage_, other.device_storage_,
                data_.size(), 0, 0, 0));
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] / other.data_[i];
    return result;
}

Tensor& Tensor::operator+=(const Tensor& other) {
    if (shape_ != other.shape_)
        throw std::invalid_argument("Tensor shapes must match for addition");
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        other.ensure_device();
        device_storage_ = detail::vulkan_tensor_execute_resident(
            detail::VulkanTensorOperation::ADD,
            device_storage_, other.device_storage_,
            data_.size(), 0, 0, 0);
        host_valid_ = false;
        return *this;
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    other.data();
    for (size_t i = 0; i < data_.size(); ++i)
        data_[i] += other.data_[i];
    return *this;
}

Tensor& Tensor::operator-=(const Tensor& other) {
    if (shape_ != other.shape_)
        throw std::invalid_argument("Tensor shapes must match for subtraction");
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        other.ensure_device();
        device_storage_ = detail::vulkan_tensor_execute_resident(
            detail::VulkanTensorOperation::SUBTRACT,
            device_storage_, other.device_storage_,
            data_.size(), 0, 0, 0);
        host_valid_ = false;
        return *this;
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    other.data();
    for (size_t i = 0; i < data_.size(); ++i)
        data_[i] -= other.data_[i];
    return *this;
}

Tensor Tensor::operator*(double scalar) const {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::SCALAR_MULTIPLY,
                device_storage_, {}, data_.size(), 0, 0, 0, scalar));
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] * scalar;
    return result;
}

Tensor Tensor::operator/(double scalar) const {
    if (std::abs(scalar) < 1e-10)
        throw std::runtime_error("Division by zero");
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(data_.size())) {
        ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::SCALAR_DIVIDE,
                device_storage_, {}, data_.size(), 0, 0, 0, scalar));
    }
#else
    detail::should_use_vulkan(data_.size());
#endif
    data();
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] / scalar;
    return result;
}

Tensor Tensor::matmul(const Tensor& other) const {
    if (shape_.size() != 2 || other.shape_.size() != 2)
        throw std::invalid_argument("Matmul requires 2D tensors");
    if (shape_[1] != other.shape_[0])
        throw std::invalid_argument("Matmul dimension mismatch");

    size_t m = shape_[0], k = shape_[1], n = other.shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(m * n)) {
        ensure_device();
        other.ensure_device();
        return device_result(
            {m, n},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::MATMUL,
                device_storage_, other.device_storage_,
                m * n, m, n, k));
    }
#else
    detail::should_use_vulkan(m * n);
#endif
    data();
    other.data();
    Tensor result({m, n}, 0.0);
    detail::parallel_for(
        m, m * n * k,
        [&](size_t row_begin, size_t row_end) {
            for (size_t i = row_begin; i < row_end; ++i)
                for (size_t j = 0; j < n; ++j)
                    for (size_t p = 0; p < k; ++p)
                        result.data_[i * n + j] +=
                            data_[i * k + p] *
                            other.data_[p * n + j];
        });
    return result;
}

Tensor Tensor::matmul_bias(
    const Tensor& weights,
    const Tensor& bias) const {
    if (ndim() != 2 || weights.ndim() != 2 ||
        bias.ndim() != 1 || shape_[1] != weights.shape_[0] ||
        weights.shape_[1] != bias.size()) {
        throw std::invalid_argument(
            "matmul_bias dimensions are inconsistent");
    }
    const size_t rows = shape_[0];
    const size_t inner = shape_[1];
    const size_t columns = weights.shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(rows * columns)) {
        ensure_device();
        weights.ensure_device();
        bias.ensure_device();
        return device_result(
            {rows, columns},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::MATMUL_BIAS,
                device_storage_, weights.device_storage_,
                rows * columns, rows, columns, inner, 0.0,
                bias.device_storage_));
    }
#endif
    Tensor result = matmul(weights);
    result.data();
    for (size_t row = 0; row < rows; ++row)
        for (size_t column = 0; column < columns; ++column)
            result.data_[row * columns + column] += bias.data()[column];
    return result;
}

Tensor Tensor::relu() const {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::RELU,
                device_storage_, {}, size(), 0, 0, 0));
    }
#endif
    Tensor result(shape_);
    for (size_t index = 0; index < size(); ++index)
        result.data_[index] = std::max(0.0, data()[index]);
    return result;
}

Tensor Tensor::relu_backward(const Tensor& gradient) const {
    if (shape_ != gradient.shape_) {
        throw std::invalid_argument(
            "ReLU gradient shape does not match input");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        gradient.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::RELU_BACKWARD,
                device_storage_, gradient.device_storage_,
                size(), 0, 0, 0));
    }
#endif
    Tensor result(shape_);
    for (size_t index = 0; index < size(); ++index)
        result.data_[index] =
            data()[index] > 0.0 ? gradient.data()[index] : 0.0;
    return result;
}

Tensor Tensor::gelu() const {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::GELU,
                device_storage_, {}, size(), 0, 0, 0));
    }
#endif
    Tensor result(shape_);
    constexpr double coefficient = 0.7978845608028654;
    for (size_t index = 0; index < size(); ++index) {
        const double value = data()[index];
        result.data_[index] =
            0.5 * value *
            (1.0 + std::tanh(
                coefficient *
                (value + 0.044715 * value * value * value)));
    }
    return result;
}

Tensor Tensor::gelu_backward(const Tensor& gradient) const {
    if (shape_ != gradient.shape_) {
        throw std::invalid_argument(
            "GELU gradient shape does not match input");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        gradient.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::GELU_BACKWARD,
                device_storage_, gradient.device_storage_,
                size(), 0, 0, 0));
    }
#endif
    Tensor result(shape_);
    constexpr double coefficient = 0.7978845608028654;
    for (size_t index = 0; index < size(); ++index) {
        const double value = data()[index];
        const double cubic =
            coefficient *
            (value + 0.044715 * value * value * value);
        const double tangent = std::tanh(cubic);
        const double derivative =
            0.5 * (1.0 + tangent) +
            0.5 * value * (1.0 - tangent * tangent) *
            coefficient * (1.0 + 0.134145 * value * value);
        result.data_[index] =
            gradient.data()[index] * derivative;
    }
    return result;
}

Tensor Tensor::silu() const {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::SILU,
                device_storage_, {}, size(), 0, 0, 0));
    }
#endif
    Tensor result(shape_);
    for (size_t index = 0; index < size(); ++index) {
        const double value = data()[index];
        result.data_[index] =
            value / (1.0 + std::exp(-value));
    }
    return result;
}

Tensor Tensor::silu_backward(const Tensor& gradient) const {
    if (shape_ != gradient.shape_) {
        throw std::invalid_argument(
            "SiLU gradient shape does not match input");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        gradient.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::SILU_BACKWARD,
                device_storage_, gradient.device_storage_,
                size(), 0, 0, 0));
    }
#endif
    Tensor result(shape_);
    for (size_t index = 0; index < size(); ++index) {
        const double value = data()[index];
        const double sigmoid = 1.0 / (1.0 + std::exp(-value));
        result.data_[index] = gradient.data()[index] *
            sigmoid * (1.0 + value * (1.0 - sigmoid));
    }
    return result;
}

Tensor Tensor::rotary_embedding(size_t num_heads) const {
    if (ndim() != 2 || num_heads == 0 ||
        shape_[1] % num_heads != 0 ||
        (shape_[1] / num_heads) % 2 != 0) {
        throw std::invalid_argument(
            "RoPE requires [sequence, width] with even head width");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::ROTARY_EMBEDDING,
                device_storage_, {}, size(),
                shape_[0], shape_[1], 0,
                static_cast<double>(num_heads)));
    }
#endif
    Tensor result(shape_);
    const size_t head_width = shape_[1] / num_heads;
    for (size_t row = 0; row < shape_[0]; ++row) {
        for (size_t column = 0; column < shape_[1]; column += 2) {
            const size_t pair = (column % head_width) & ~size_t{1};
            const double angle = static_cast<double>(row) /
                std::pow(
                    10000.0,
                    static_cast<double>(pair) /
                        static_cast<double>(head_width));
            const double cosine = std::cos(angle);
            const double sine = std::sin(angle);
            const size_t base = row * shape_[1] + column;
            result.data_[base] =
                data()[base] * cosine -
                data()[base + 1] * sine;
            result.data_[base + 1] =
                data()[base] * sine +
                data()[base + 1] * cosine;
        }
    }
    return result;
}

Tensor Tensor::rotary_embedding_backward(size_t num_heads) const {
    if (ndim() != 2 || num_heads == 0 ||
        shape_[1] % num_heads != 0 ||
        (shape_[1] / num_heads) % 2 != 0) {
        throw std::invalid_argument(
            "RoPE backward requires even per-head width");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::
                    ROTARY_EMBEDDING_BACKWARD,
                device_storage_, {}, size(),
                shape_[0], shape_[1], 0,
                static_cast<double>(num_heads)));
    }
#endif
    Tensor result(shape_);
    const size_t head_width = shape_[1] / num_heads;
    for (size_t row = 0; row < shape_[0]; ++row) {
        for (size_t column = 0; column < shape_[1]; column += 2) {
            const size_t pair = (column % head_width) & ~size_t{1};
            const double angle = -static_cast<double>(row) /
                std::pow(
                    10000.0,
                    static_cast<double>(pair) /
                        static_cast<double>(head_width));
            const double cosine = std::cos(angle);
            const double sine = std::sin(angle);
            const size_t base = row * shape_[1] + column;
            result.data_[base] =
                data()[base] * cosine -
                data()[base + 1] * sine;
            result.data_[base + 1] =
                data()[base] * sine +
                data()[base + 1] * cosine;
        }
    }
    return result;
}

Tensor Tensor::row_softmax() const {
    if (ndim() != 2) {
        throw std::invalid_argument(
            "row_softmax requires a 2D tensor");
    }
    const size_t rows = shape_[0];
    const size_t columns = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::ROW_SOFTMAX,
                device_storage_, {}, size(), rows, columns, 0));
    }
#endif
    Tensor result(shape_);
    for (size_t row = 0; row < rows; ++row) {
        double maximum = -std::numeric_limits<double>::infinity();
        for (size_t column = 0; column < columns; ++column)
            maximum = std::max(
                maximum, data()[row * columns + column]);
        double total = 0.0;
        for (size_t column = 0; column < columns; ++column) {
            const double value = std::exp(
                data()[row * columns + column] - maximum);
            result.data_[row * columns + column] = value;
            total += value;
        }
        for (size_t column = 0; column < columns; ++column)
            result.data_[row * columns + column] /= total;
    }
    return result;
}

Tensor Tensor::masked_fill(
    const Tensor& mask,
    double fill_value) const {
    if (shape_ != mask.shape_) {
        throw std::invalid_argument(
            "masked_fill mask shape does not match tensor");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size()) &&
        fill_value == -1e9) {
        ensure_device();
        mask.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::MASK_FILL,
                device_storage_, mask.device_storage_,
                size(), 0, 0, 0, fill_value));
    }
#endif
    Tensor result = clone();
    result.data();
    for (size_t index = 0; index < size(); ++index)
        if (mask.data()[index] == 0.0)
            result.data_[index] = fill_value;
    return result;
}

Tensor Tensor::layer_norm(
    const Tensor& gamma,
    const Tensor& beta,
    double epsilon) const {
    if (ndim() != 2 || gamma.ndim() != 1 ||
        beta.shape_ != gamma.shape_ ||
        shape_[1] != gamma.size()) {
        throw std::invalid_argument(
            "layer_norm dimensions are inconsistent");
    }
    const size_t rows = shape_[0];
    const size_t columns = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        gamma.ensure_device();
        beta.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::LAYER_NORM,
                device_storage_, gamma.device_storage_,
                size(), rows, columns, 0, epsilon,
                beta.device_storage_));
    }
#endif
    Tensor result(shape_);
    for (size_t row = 0; row < rows; ++row) {
        double mean = 0.0;
        for (size_t column = 0; column < columns; ++column)
            mean += data()[row * columns + column];
        mean /= static_cast<double>(columns);
        double variance = 0.0;
        for (size_t column = 0; column < columns; ++column) {
            const double difference =
                data()[row * columns + column] - mean;
            variance += difference * difference;
        }
        const double inverse = 1.0 / std::sqrt(
            variance / static_cast<double>(columns) + epsilon);
        for (size_t column = 0; column < columns; ++column)
            result.data_[row * columns + column] =
                (data()[row * columns + column] - mean) * inverse *
                    gamma.data()[column] +
                beta.data()[column];
    }
    return result;
}

Tensor Tensor::layer_norm_backward(
    const Tensor& gradient,
    const Tensor& gamma,
    double epsilon) const {
    if (shape_ != gradient.shape_ || ndim() != 2 ||
        gamma.size() != shape_[1]) {
        throw std::invalid_argument(
            "layer_norm_backward dimensions are inconsistent");
    }
    const size_t rows = shape_[0];
    const size_t columns = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        gradient.ensure_device();
        gamma.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::LAYER_NORM_BACKWARD,
                device_storage_, gradient.device_storage_,
                size(), rows, columns, 0, epsilon,
                gamma.device_storage_));
    }
#endif
    Tensor normalized = layer_norm(
        Tensor({columns}, 1.0),
        Tensor({columns}, 0.0), epsilon);
    Tensor result(shape_);
    for (size_t row = 0; row < rows; ++row) {
        double variance = 0.0;
        double mean = 0.0;
        for (size_t column = 0; column < columns; ++column)
            mean += data()[row * columns + column];
        mean /= static_cast<double>(columns);
        for (size_t column = 0; column < columns; ++column) {
            const double difference =
                data()[row * columns + column] - mean;
            variance += difference * difference;
        }
        const double inverse = 1.0 / std::sqrt(
            variance / static_cast<double>(columns) + epsilon);
        double sum_dy = 0.0;
        double sum_dy_normalized = 0.0;
        for (size_t column = 0; column < columns; ++column) {
            const double dy =
                gradient.data()[row * columns + column] *
                gamma.data()[column];
            sum_dy += dy;
            sum_dy_normalized +=
                dy * normalized.data()[row * columns + column];
        }
        for (size_t column = 0; column < columns; ++column) {
            const double dy =
                gradient.data()[row * columns + column] *
                gamma.data()[column];
            result.data_[row * columns + column] =
                inverse / static_cast<double>(columns) *
                (static_cast<double>(columns) * dy - sum_dy -
                 normalized.data()[row * columns + column] *
                    sum_dy_normalized);
        }
    }
    return result;
}

Tensor Tensor::layer_norm_gamma_gradient(
    const Tensor& gradient,
    double epsilon) const {
    if (shape_ != gradient.shape_ || ndim() != 2) {
        throw std::invalid_argument(
            "layer_norm_gamma_gradient dimensions are inconsistent");
    }
    const size_t rows = shape_[0];
    const size_t columns = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        gradient.ensure_device();
        return device_result(
            {columns},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::
                    LAYER_NORM_GAMMA_GRADIENT,
                device_storage_, gradient.device_storage_,
                columns, rows, columns, 0, epsilon));
    }
#endif
    Tensor normalized = layer_norm(
        Tensor({columns}, 1.0),
        Tensor({columns}, 0.0), epsilon);
    Tensor result({columns}, 0.0);
    for (size_t row = 0; row < rows; ++row)
        for (size_t column = 0; column < columns; ++column)
            result.data_[column] +=
                gradient.data()[row * columns + column] *
                normalized.data()[row * columns + column];
    return result;
}

Tensor Tensor::row_sum() const {
    if (ndim() != 2) {
        throw std::invalid_argument("row_sum requires a 2D tensor");
    }
    const size_t rows = shape_[0];
    const size_t columns = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        return device_result(
            {columns},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::ROW_SUM,
                device_storage_, {}, columns, rows, columns, 0));
    }
#endif
    Tensor result({columns}, 0.0);
    for (size_t row = 0; row < rows; ++row)
        for (size_t column = 0; column < columns; ++column)
            result.data_[column] += data()[row * columns + column];
    return result;
}

Tensor Tensor::broadcast_add(const Tensor& row) const {
    if (ndim() != 2 || row.ndim() != 1 ||
        row.size() != shape_[1]) {
        throw std::invalid_argument(
            "broadcast_add dimensions are inconsistent");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        row.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::BROADCAST_ADD,
                device_storage_, row.device_storage_,
                size(), shape_[0], shape_[1], 0));
    }
#endif
    Tensor result(shape_);
    for (size_t index = 0; index < size(); ++index)
        result.data_[index] =
            data()[index] + row.data()[index % row.size()];
    return result;
}

Tensor Tensor::broadcast_multiply(const Tensor& row) const {
    if (ndim() != 2 || row.ndim() != 1 ||
        row.size() != shape_[1]) {
        throw std::invalid_argument(
            "broadcast_multiply dimensions are inconsistent");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        row.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::BROADCAST_MULTIPLY,
                device_storage_, row.device_storage_,
                size(), shape_[0], shape_[1], 0));
    }
#endif
    Tensor result(shape_);
    for (size_t index = 0; index < size(); ++index)
        result.data_[index] =
            data()[index] * row.data()[index % row.size()];
    return result;
}

Tensor Tensor::add_prefix(const Tensor& prefix) const {
    if (ndim() != 2 || prefix.ndim() != 2 ||
        prefix.shape_[0] < shape_[0] ||
        prefix.shape_[1] != shape_[1]) {
        throw std::invalid_argument(
            "add_prefix dimensions are inconsistent");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        prefix.ensure_device();
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::ADD_PREFIX,
                device_storage_, prefix.device_storage_,
                size(), shape_[0], shape_[1], 0));
    }
#endif
    Tensor result(shape_);
    for (size_t index = 0; index < size(); ++index)
        result.data_[index] = data()[index] + prefix.data()[index];
    return result;
}

Tensor Tensor::slice_columns(size_t offset, size_t width) const {
    if (ndim() != 2 || offset + width > shape_[1]) {
        throw std::invalid_argument(
            "slice_columns range exceeds tensor shape");
    }
    const size_t rows = shape_[0];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(rows * width)) {
        ensure_device();
        return device_result(
            {rows, width},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::SLICE_COLUMNS,
                device_storage_, {}, rows * width,
                rows, width, shape_[1],
                static_cast<double>(offset)));
    }
#endif
    Tensor result({rows, width});
    for (size_t row = 0; row < rows; ++row)
        for (size_t column = 0; column < width; ++column)
            result.data_[row * width + column] =
                data()[row * shape_[1] + offset + column];
    return result;
}

Tensor Tensor::concat_columns(const Tensor& other) const {
    if (ndim() != 2 || other.ndim() != 2 ||
        shape_[0] != other.shape_[0]) {
        throw std::invalid_argument(
            "concat_columns row counts must match");
    }
    const size_t rows = shape_[0];
    const size_t left_width = shape_[1];
    const size_t right_width = other.shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(
            rows * (left_width + right_width))) {
        ensure_device();
        other.ensure_device();
        return device_result(
            {rows, left_width + right_width},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::CONCAT_COLUMNS,
                device_storage_, other.device_storage_,
                rows * (left_width + right_width),
                rows, left_width, right_width));
    }
#endif
    Tensor result({rows, left_width + right_width});
    for (size_t row = 0; row < rows; ++row) {
        std::copy_n(
            data().begin() + row * left_width,
            left_width,
            result.data_.begin() +
                row * (left_width + right_width));
        std::copy_n(
            other.data().begin() + row * right_width,
            right_width,
            result.data_.begin() +
                row * (left_width + right_width) + left_width);
    }
    return result;
}

Tensor Tensor::cross_entropy_with_one_hot(
    const Tensor& one_hot_targets) const {
    if (ndim() != 2 || shape_ != one_hot_targets.shape_) {
        throw std::invalid_argument(
            "Cross-entropy logits and targets must be matching matrices");
    }
    const size_t rows = shape_[0];
    const size_t columns = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(rows * columns)) {
        ensure_device();
        one_hot_targets.ensure_device();
        return device_result(
            {rows, columns + 1},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::CROSS_ENTROPY,
                device_storage_,
                one_hot_targets.device_storage_,
                rows * (columns + 1),
                rows,
                columns,
                0));
    }
#endif
    Tensor result({rows, columns + 1}, 0.0);
    for (size_t row = 0; row < rows; ++row) {
        const size_t input_base = row * columns;
        const size_t output_base = row * (columns + 1);
        double maximum = -std::numeric_limits<double>::infinity();
        for (size_t column = 0; column < columns; ++column) {
            maximum = std::max(
                maximum, data()[input_base + column]);
        }
        double denominator = 0.0;
        for (size_t column = 0; column < columns; ++column) {
            const double probability =
                std::exp(data()[input_base + column] - maximum);
            result.data_[output_base + column] = probability;
            denominator += probability;
        }
        double target_probability = 0.0;
        for (size_t column = 0; column < columns; ++column) {
            const double probability =
                result.data_[output_base + column] / denominator;
            result.data_[output_base + column] =
                (probability -
                 one_hot_targets.data()[input_base + column]) /
                static_cast<double>(rows);
            target_probability +=
                probability *
                one_hot_targets.data()[input_base + column];
        }
        result.data_[output_base + columns] =
            -std::log(std::max(
                target_probability,
                std::numeric_limits<double>::min()));
    }
    return result;
}

Tensor Tensor::attention_scores(
    const Tensor& keys,
    const Tensor& mask,
    double scale) const {
    if (ndim() != 2 || keys.ndim() != 2 ||
        shape_[1] != keys.shape_[1] ||
        mask.shape_ !=
            std::vector<size_t>{shape_[0], keys.shape_[0]} ||
        scale <= 0.0) {
        throw std::invalid_argument(
            "Attention score dimensions or scale are invalid");
    }
    const size_t query_rows = shape_[0];
    const size_t key_rows = keys.shape_[0];
    const size_t width = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(query_rows * key_rows)) {
        ensure_device();
        keys.ensure_device();
        mask.ensure_device();
        return device_result(
            {query_rows, key_rows},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::ATTENTION_SCORES,
                device_storage_,
                keys.device_storage_,
                query_rows * key_rows,
                query_rows,
                key_rows,
                width,
                scale,
                mask.device_storage_));
    }
#endif
    Tensor result({query_rows, key_rows}, 0.0);
    for (size_t query = 0; query < query_rows; ++query) {
        for (size_t key = 0; key < key_rows; ++key) {
            const size_t output_index = query * key_rows + key;
            if (mask.data()[output_index] == 0.0) {
                result.data_[output_index] = -1e9;
                continue;
            }
            double total = 0.0;
            for (size_t column = 0; column < width; ++column) {
                total +=
                    data()[query * width + column] *
                    keys.data()[key * width + column];
            }
            result.data_[output_index] = total / scale;
        }
    }
    return result;
}

Tensor Tensor::matmul_right_transposed(const Tensor& other) const {
    if (ndim() != 2 || other.ndim() != 2 ||
        shape_[1] != other.shape_[1]) {
        throw std::invalid_argument(
            "Right-transposed matmul inner dimensions must match");
    }
    const size_t rows = shape_[0];
    const size_t columns = other.shape_[0];
    const size_t inner = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(rows * columns * inner)) {
        ensure_device();
        other.ensure_device();
        return device_result(
            {rows, columns},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::
                    MATMUL_RIGHT_TRANSPOSED,
                device_storage_,
                other.device_storage_,
                rows * columns,
                rows,
                columns,
                inner));
    }
#endif
    Tensor result({rows, columns}, 0.0);
    for (size_t row = 0; row < rows; ++row)
        for (size_t column = 0; column < columns; ++column)
            for (size_t index = 0; index < inner; ++index)
                result.data_[row * columns + column] +=
                    data()[row * inner + index] *
                    other.data()[column * inner + index];
    return result;
}

Tensor Tensor::matmul_left_transposed(const Tensor& other) const {
    if (ndim() != 2 || other.ndim() != 2 ||
        shape_[0] != other.shape_[0]) {
        throw std::invalid_argument(
            "Left-transposed matmul row dimensions must match");
    }
    const size_t rows = shape_[1];
    const size_t columns = other.shape_[1];
    const size_t inner = shape_[0];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(rows * columns * inner)) {
        ensure_device();
        other.ensure_device();
        return device_result(
            {rows, columns},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::
                    MATMUL_LEFT_TRANSPOSED,
                device_storage_,
                other.device_storage_,
                rows * columns,
                rows,
                columns,
                inner));
    }
#endif
    Tensor result({rows, columns}, 0.0);
    for (size_t row = 0; row < rows; ++row)
        for (size_t column = 0; column < columns; ++column)
            for (size_t index = 0; index < inner; ++index)
                result.data_[row * columns + column] +=
                    data()[index * rows + row] *
                    other.data()[index * columns + column];
    return result;
}

Tensor Tensor::multihead_attention_scores(
    const Tensor& keys,
    const Tensor& mask,
    size_t heads) const {
    if (ndim() != 2 || keys.shape_ != shape_ ||
        shape_[0] == 0 || heads == 0 ||
        shape_[1] % heads != 0 ||
        mask.shape_ !=
            std::vector<size_t>{shape_[0], shape_[0]}) {
        throw std::invalid_argument(
            "Packed multi-head attention dimensions are invalid");
    }
    const size_t sequence = shape_[0];
    const size_t model = shape_[1];
    const size_t head_width = model / heads;
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(
            heads * sequence * sequence * head_width)) {
        ensure_device();
        keys.ensure_device();
        mask.ensure_device();
        return device_result(
            {heads * sequence, sequence},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::
                    MULTIHEAD_ATTENTION_SCORES,
                device_storage_,
                keys.device_storage_,
                heads * sequence * sequence,
                sequence,
                model,
                heads,
                std::sqrt(static_cast<double>(head_width)),
                mask.device_storage_));
    }
#endif
    Tensor result({heads * sequence, sequence}, 0.0);
    const double scale = std::sqrt(
        static_cast<double>(head_width));
    for (size_t head = 0; head < heads; ++head)
        for (size_t query = 0; query < sequence; ++query)
            for (size_t key = 0; key < sequence; ++key) {
                const size_t output =
                    (head * sequence + query) * sequence + key;
                if (mask.data()[query * sequence + key] == 0.0) {
                    result.data_[output] = -1e9;
                    continue;
                }
                for (size_t column = 0;
                     column < head_width;
                     ++column) {
                    const size_t model_column =
                        head * head_width + column;
                    result.data_[output] +=
                        data()[query * model + model_column] *
                        keys.data()[key * model + model_column];
                }
                result.data_[output] /= scale;
            }
    return result;
}

Tensor Tensor::multihead_attention_values(
    const Tensor& values,
    size_t heads) const {
    if (ndim() != 2 || values.ndim() != 2 ||
        heads == 0 || values.shape_[1] % heads != 0 ||
        shape_[0] != heads * values.shape_[0] ||
        shape_[1] != values.shape_[0]) {
        throw std::invalid_argument(
            "Packed attention weights and values are incompatible");
    }
    const size_t sequence = values.shape_[0];
    const size_t model = values.shape_[1];
    const size_t head_width = model / heads;
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(sequence * sequence * model)) {
        ensure_device();
        values.ensure_device();
        return device_result(
            {sequence, model},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::
                    MULTIHEAD_ATTENTION_VALUES,
                device_storage_,
                values.device_storage_,
                sequence * model,
                sequence,
                model,
                heads));
    }
#endif
    Tensor result({sequence, model}, 0.0);
    for (size_t query = 0; query < sequence; ++query)
        for (size_t column = 0; column < model; ++column) {
            const size_t head = column / head_width;
            for (size_t key = 0; key < sequence; ++key)
                result.data_[query * model + column] +=
                    data()[
                        (head * sequence + query) *
                            sequence + key] *
                    values.data()[key * model + column];
        }
    return result;
}

Tensor Tensor::multihead_attention_key_gradient(
    const Tensor& queries,
    size_t heads) const {
    if (ndim() != 2 || queries.ndim() != 2 ||
        heads == 0 || queries.shape_[1] % heads != 0 ||
        shape_[0] != heads * queries.shape_[0] ||
        shape_[1] != queries.shape_[0]) {
        throw std::invalid_argument(
            "Packed attention weights and queries are incompatible");
    }
    const size_t sequence = queries.shape_[0];
    const size_t model = queries.shape_[1];
    const size_t head_width = model / heads;
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(sequence * sequence * model)) {
        ensure_device();
        queries.ensure_device();
        return device_result(
            {sequence, model},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::
                    MULTIHEAD_ATTENTION_KEY_GRADIENT,
                device_storage_,
                queries.device_storage_,
                sequence * model,
                sequence,
                model,
                heads));
    }
#endif
    Tensor result({sequence, model}, 0.0);
    for (size_t key = 0; key < sequence; ++key)
        for (size_t column = 0; column < model; ++column) {
            const size_t head = column / head_width;
            for (size_t query = 0; query < sequence; ++query)
                result.data_[key * model + column] +=
                    data()[
                        (head * sequence + query) *
                            sequence + key] *
                    queries.data()[query * model + column];
        }
    return result;
}

Tensor Tensor::multihead_attention_weight_gradient(
    const Tensor& values,
    size_t heads) const {
    if (ndim() != 2 || values.shape_ != shape_ ||
        shape_[0] == 0 || heads == 0 ||
        shape_[1] % heads != 0) {
        throw std::invalid_argument(
            "Packed attention gradient dimensions are invalid");
    }
    const size_t sequence = shape_[0];
    const size_t model = shape_[1];
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(sequence * sequence * model)) {
        ensure_device();
        values.ensure_device();
        return device_result(
            {heads * sequence, sequence},
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::
                    MULTIHEAD_ATTENTION_WEIGHT_GRADIENT,
                device_storage_,
                values.device_storage_,
                heads * sequence * sequence,
                sequence,
                model,
                heads));
    }
#endif
    const size_t head_width = model / heads;
    Tensor result({heads * sequence, sequence}, 0.0);
    for (size_t head = 0; head < heads; ++head)
        for (size_t query = 0; query < sequence; ++query)
            for (size_t key = 0; key < sequence; ++key)
                for (size_t column = 0;
                     column < head_width;
                     ++column) {
                    const size_t model_column =
                        head * head_width + column;
                    result.data_[
                        (head * sequence + query) *
                            sequence + key] +=
                        data()[query * model + model_column] *
                        values.data()[key * model + model_column];
                }
    return result;
}

Tensor Tensor::squared_sum_tensor() const {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        detail::VulkanTensorStoragePtr partials =
            device_storage_;
        size_t input_size = size();
        bool square_values = true;
        do {
            const size_t output_size =
                (input_size + 255) / 256;
            partials =
                detail::vulkan_tensor_execute_resident(
                    square_values
                        ? detail::VulkanTensorOperation::SQUARED_SUM
                        : detail::VulkanTensorOperation::SUM_REDUCTION,
                    partials,
                    {},
                    output_size,
                    input_size,
                    0,
                    0);
            input_size = output_size;
            square_values = false;
        } while (input_size > 1);
        return device_result(
            {1},
            std::move(partials));
    }
#endif
    double result = 0.0;
    for (double value : data()) {
        result += value * value;
    }
    return Tensor({1}, result);
}

Tensor Tensor::sum_tensor() const {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        detail::VulkanTensorStoragePtr partials =
            device_storage_;
        size_t input_size = size();
        do {
            const size_t output_size =
                (input_size + 255) / 256;
            partials =
                detail::vulkan_tensor_execute_resident(
                    detail::VulkanTensorOperation::SUM_REDUCTION,
                    partials,
                    {},
                    output_size,
                    input_size,
                    0,
                    0);
            input_size = output_size;
        } while (input_size > 1);
        return device_result({1}, std::move(partials));
    }
#endif
    double result = 0.0;
    for (double value : data()) {
        result += value;
    }
    return Tensor({1}, result);
}

double Tensor::squared_sum() const {
    return squared_sum_tensor().data()[0];
}

Tensor Tensor::adamw_update(
    const Tensor& first_moment,
    const Tensor& second_moment,
    double learning_rate,
    double beta1_correction,
    double beta2_correction,
    double epsilon) const {
    if (shape_ != first_moment.shape_ ||
        shape_ != second_moment.shape_) {
        throw std::invalid_argument(
            "AdamW state shape does not match parameter");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        first_moment.ensure_device();
        second_moment.ensure_device();
        const float beta1_value =
            static_cast<float>(beta1_correction);
        const float beta2_value =
            static_cast<float>(beta2_correction);
        const float epsilon_value = static_cast<float>(epsilon);
        std::uint32_t beta1_bits = 0;
        std::uint32_t beta2_bits = 0;
        std::uint32_t epsilon_bits = 0;
        std::memcpy(&beta1_bits, &beta1_value, sizeof(beta1_bits));
        std::memcpy(&beta2_bits, &beta2_value, sizeof(beta2_bits));
        std::memcpy(&epsilon_bits, &epsilon_value, sizeof(epsilon_bits));
        return device_result(
            shape_,
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::ADAMW_UPDATE,
                device_storage_, first_moment.device_storage_,
                size(), beta1_bits, beta2_bits, epsilon_bits,
                learning_rate, second_moment.device_storage_));
    }
#endif
    Tensor result(shape_);
    for (size_t index = 0; index < size(); ++index) {
        const double update =
            (first_moment.data()[index] / beta1_correction) /
            (std::sqrt(
                 second_moment.data()[index] / beta2_correction) +
             epsilon);
        result.data_[index] =
            data()[index] - learning_rate * update;
    }
    return result;
}

void Tensor::adamw_update_in_place(
    const Tensor& first_moment,
    const Tensor& second_moment,
    double learning_rate,
    double beta1_correction,
    double beta2_correction,
    double epsilon) {
    if (shape_ != first_moment.shape_ ||
        shape_ != second_moment.shape_) {
        throw std::invalid_argument(
            "AdamW state shape does not match parameter");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        first_moment.ensure_device();
        second_moment.ensure_device();
        const float beta1_value =
            static_cast<float>(beta1_correction);
        const float beta2_value =
            static_cast<float>(beta2_correction);
        const float epsilon_value = static_cast<float>(epsilon);
        std::uint32_t beta1_bits = 0;
        std::uint32_t beta2_bits = 0;
        std::uint32_t epsilon_bits = 0;
        std::memcpy(&beta1_bits, &beta1_value, sizeof(beta1_bits));
        std::memcpy(&beta2_bits, &beta2_value, sizeof(beta2_bits));
        std::memcpy(&epsilon_bits, &epsilon_value, sizeof(epsilon_bits));
        device_storage_ =
            detail::vulkan_tensor_execute_resident(
                detail::VulkanTensorOperation::ADAMW_UPDATE,
                device_storage_, first_moment.device_storage_,
                size(), beta1_bits, beta2_bits, epsilon_bits,
                learning_rate, second_moment.device_storage_);
        host_valid_ = false;
        return;
    }
#endif
    for (size_t index = 0; index < size(); ++index) {
        const double update =
            (first_moment.data()[index] / beta1_correction) /
            (std::sqrt(
                 second_moment.data()[index] / beta2_correction) +
             epsilon);
        data()[index] -= learning_rate * update;
    }
}

void Tensor::fused_adamw_step_in_place(
    const Tensor& gradients,
    Tensor& first_moment,
    Tensor& second_moment,
    double learning_rate,
    double beta1,
    double beta2,
    double beta1_correction,
    double beta2_correction,
    double epsilon,
    double weight_decay,
    double gradient_scale) {
    if (shape_ != gradients.shape_ ||
        shape_ != first_moment.shape_ ||
        shape_ != second_moment.shape_) {
        throw std::invalid_argument(
            "Fused AdamW tensors must have matching shapes");
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (detail::should_use_vulkan(size())) {
        ensure_device();
        gradients.ensure_device();
        first_moment.ensure_device();
        second_moment.ensure_device();
        detail::vulkan_tensor_adamw_step_in_place(
            device_storage_,
            gradients.device_storage_,
            first_moment.device_storage_,
            second_moment.device_storage_,
            learning_rate,
            beta1,
            beta2,
            beta1_correction,
            beta2_correction,
            epsilon,
            weight_decay,
            gradient_scale);
        host_valid_ = false;
        first_moment.host_valid_ = false;
        second_moment.host_valid_ = false;
        return;
    }
#endif
    for (size_t index = 0; index < size(); ++index) {
        const double gradient =
            gradients.data()[index] * gradient_scale;
        double& first = first_moment.data()[index];
        double& second = second_moment.data()[index];
        first = beta1 * first + (1.0 - beta1) * gradient;
        second =
            beta2 * second +
            (1.0 - beta2) * gradient * gradient;
        const double update =
            (first / beta1_correction) /
            (std::sqrt(second / beta2_correction) + epsilon);
        data()[index] -= learning_rate * update;
        data()[index] -=
            learning_rate * weight_decay * data()[index];
    }
}

void Tensor::fill(double value) {
    data();
    std::fill(data_.begin(), data_.end(), value);
}

void Tensor::randomize(double min, double max) {
    data();
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(min, max);
    for (auto& v : data_) v = dist(gen);
}

Tensor Tensor::clone() const {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (device_storage_) {
        Tensor result(shape_);
        result.data_ = data_;
        result.device_storage_ = device_storage_;
        result.host_valid_ = host_valid_;
        return result;
    }
#endif
    return Tensor(shape_, data());
}

double Tensor::sum() const {
    data();
    double s = 0.0;
    for (auto v : data_) s += v;
    return s;
}

double Tensor::mean() const {
    if (data_.empty()) return 0.0;
    return sum() / static_cast<double>(data_.size());
}

} // namespace deep_learning
} // namespace ml
