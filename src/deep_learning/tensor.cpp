#include "deep_learning/tensor.h"
#include <random>
#include <sstream>

namespace ml {
namespace deep_learning {

Tensor::Tensor(const std::vector<size_t>& shape) : shape_(shape) {
    size_t total_size = 1;
    for (auto dim : shape) {
        total_size *= dim;
    }
    data_.resize(total_size, 0.0);
}

Tensor::Tensor(const std::vector<size_t>& shape, double fill_value) : shape_(shape) {
    size_t total_size = 1;
    for (auto dim : shape) {
        total_size *= dim;
    }
    data_.resize(total_size, fill_value);
}

Tensor::Tensor(const std::vector<size_t>& shape, const std::vector<double>& data) 
    : shape_(shape), data_(data) {
    size_t total_size = 1;
    for (auto dim : shape) {
        total_size *= dim;
    }
    if (data.size() != total_size) {
        throw std::invalid_argument("Data size does not match shape");
    }
}

size_t Tensor::compute_index(const std::vector<size_t>& indices) const {
    if (indices.size() != shape_.size()) {
        throw std::invalid_argument("Number of indices does not match tensor dimensions");
    }
    
    size_t index = 0;
    size_t multiplier = 1;
    for (int i = shape_.size() - 1; i >= 0; --i) {
        if (indices[i] >= shape_[i]) {
            throw std::out_of_range("Index out of bounds");
        }
        index += indices[i] * multiplier;
        multiplier *= shape_[i];
    }
    return index;
}

double& Tensor::operator()(const std::vector<size_t>& indices) {
    return data_[compute_index(indices)];
}

const double& Tensor::operator()(const std::vector<size_t>& indices) const {
    return data_[compute_index(indices)];
}

Tensor Tensor::reshape(const std::vector<size_t>& new_shape) const {
    size_t new_size = 1;
    for (auto dim : new_shape) {
        new_size *= dim;
    }
    
    if (new_size != data_.size()) {
        throw std::invalid_argument("New shape must have the same total size");
    }
    
    return Tensor(new_shape, data_);
}

Tensor Tensor::transpose() const {
    if (shape_.size() != 2) {
        throw std::invalid_argument("Transpose only supported for 2D tensors");
    }
    
    size_t rows = shape_[0];
    size_t cols = shape_[1];
    std::vector<size_t> new_shape = {cols, rows};
    Tensor result(new_shape);
    
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            result.data_[j * rows + i] = data_[i * cols + j];
        }
    }
    
    return result;
}

bool Tensor::is_shape_compatible(const Tensor& other) const {
    return shape_ == other.shape_;
}

Tensor Tensor::operator+(const Tensor& other) const {
    if (!is_shape_compatible(other)) {
        throw std::invalid_argument("Tensor shapes must match for addition");
    }
    
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] + other.data_[i];
    }
    return result;
}

Tensor Tensor::operator-(const Tensor& other) const {
    if (!is_shape_compatible(other)) {
        throw std::invalid_argument("Tensor shapes must match for subtraction");
    }
    
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] - other.data_[i];
    }
    return result;
}

Tensor Tensor::operator*(const Tensor& other) const {
    if (!is_shape_compatible(other)) {
        throw std::invalid_argument("Tensor shapes must match for element-wise multiplication");
    }
    
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] * other.data_[i];
    }
    return result;
}

Tensor Tensor::operator/(const Tensor& other) const {
    if (!is_shape_compatible(other)) {
        throw std::invalid_argument("Tensor shapes must match for element-wise division");
    }
    
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        if (std::abs(other.data_[i]) < 1e-10) {
            throw std::runtime_error("Division by zero");
        }
        result.data_[i] = data_[i] / other.data_[i];
    }
    return result;
}

Tensor& Tensor::operator+=(const Tensor& other) {
    if (!is_shape_compatible(other)) {
        throw std::invalid_argument("Tensor shapes must match for addition");
    }
    
    for (size_t i = 0; i < data_.size(); ++i) {
        data_[i] += other.data_[i];
    }
    return *this;
}

Tensor& Tensor::operator-=(const Tensor& other) {
    if (!is_shape_compatible(other)) {
        throw std::invalid_argument("Tensor shapes must match for subtraction");
    }
    
    for (size_t i = 0; i < data_.size(); ++i) {
        data_[i] -= other.data_[i];
    }
    return *this;
}

Tensor Tensor::operator*(double scalar) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] * scalar;
    }
    return result;
}

Tensor Tensor::operator/(double scalar) const {
    if (std::abs(scalar) < 1e-10) {
        throw std::runtime_error("Division by zero");
    }
    
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] / scalar;
    }
    return result;
}

Tensor Tensor::matmul(const Tensor& other) const {
    if (shape_.size() != 2 || other.shape_.size() != 2) {
        throw std::invalid_argument("Matrix multiplication requires 2D tensors");
    }
    
    size_t m = shape_[0];
    size_t k = shape_[1];
    size_t n = other.shape_[1];
    
    if (k != other.shape_[0]) {
        throw std::invalid_argument("Invalid dimensions for matrix multiplication");
    }
    
    Tensor result({m, n});
    
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            double sum = 0.0;
            for (size_t p = 0; p < k; ++p) {
                sum += data_[i * k + p] * other.data_[p * n + j];
            }
            result.data_[i * n + j] = sum;
        }
    }
    
    return result;
}

void Tensor::fill(double value) {
    std::fill(data_.begin(), data_.end(), value);
}

void Tensor::randomize(double min, double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(min, max);
    
    for (auto& val : data_) {
        val = dist(gen);
    }
}

Tensor Tensor::clone() const {
    return Tensor(shape_, data_);
}

} // namespace deep_learning
} // namespace ml
