#include "tensor.h"
#include <random>
#include <functional>
#include <stdexcept>
#include "matrix_dense.h" // Ensure matrix library is available

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
    : shape_(shape), data_(data) {}

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
    return data_[flat_index(indices)];
}

const double& Tensor::operator()(const std::vector<size_t>& indices) const {
    return data_[flat_index(indices)];
}

Tensor Tensor::reshape(const std::vector<size_t>& new_shape) const {
    size_t total = 1;
    for (auto s : new_shape) total *= s;
    if (total != data_.size()) throw std::invalid_argument("Reshape size mismatch");
    Tensor result(new_shape, data_);
    return result;
}

Tensor Tensor::transpose() const {
    if (shape_.size() != 2) throw std::invalid_argument("Transpose only for 2D tensors");
    size_t rows = shape_[0], cols = shape_[1];
    Tensor result({cols, rows});
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            result.data_[j * rows + i] = data_[i * cols + j];
    return result;
}

Tensor Tensor::operator+(const Tensor& other) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] + other.data_[i];
    return result;
}

Tensor Tensor::operator-(const Tensor& other) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] - other.data_[i];
    return result;
}

Tensor Tensor::operator*(const Tensor& other) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] * other.data_[i];
    return result;
}

Tensor Tensor::operator/(const Tensor& other) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] / other.data_[i];
    return result;
}

Tensor& Tensor::operator+=(const Tensor& other) {
    for (size_t i = 0; i < data_.size(); ++i)
        data_[i] += other.data_[i];
    return *this;
}

Tensor& Tensor::operator-=(const Tensor& other) {
    for (size_t i = 0; i < data_.size(); ++i)
        data_[i] -= other.data_[i];
    return *this;
}

Tensor Tensor::operator*(double scalar) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i)
        result.data_[i] = data_[i] * scalar;
    return result;
}

Tensor Tensor::operator/(double scalar) const {
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
    Tensor result({m, n}, 0.0);
    for (size_t i = 0; i < m; ++i)
        for (size_t j = 0; j < n; ++j)
            for (size_t p = 0; p < k; ++p)
                result.data_[i * n + j] += data_[i * k + p] * other.data_[p * n + j];
    return result;
}

void Tensor::fill(double value) {
    std::fill(data_.begin(), data_.end(), value);
}

void Tensor::randomize(double min, double max) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(min, max);
    for (auto& v : data_) v = dist(gen);
}

Tensor Tensor::clone() const {
    return Tensor(shape_, data_);
}

double Tensor::sum() const {
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
