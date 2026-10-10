#pragma once
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <numeric>
#include <initializer_list>
#include <algorithm>

namespace mytrix {

class Tensor {
public:
    Tensor() = default;
    Tensor(const std::vector<size_t>& shape, double fill_value = 0.0)
        : shape_(shape) {
        size_t total = 1;
        for (auto d : shape) total *= d;
        data_.resize(total, fill_value);
    }
    Tensor(const std::vector<size_t>& shape, const std::vector<double>& data)
        : shape_(shape), data_(data) {
        size_t total = 1;
        for (auto d : shape) total *= d;
        if (data.size() != total)
            throw std::invalid_argument("Tensor: data size does not match shape");
    }
    size_t ndim() const { return shape_.size(); }
    const std::vector<size_t>& shape() const { return shape_; }
    size_t size() const { return data_.size(); }
    double& at(size_t flat) { return data_.at(flat); }
    const double& at(size_t flat) const { return data_.at(flat); }
    double& operator[](size_t flat) { return data_[flat]; }
    const double& operator[](size_t flat) const { return data_[flat]; }
    // Multi-index access
    double& operator()(const std::vector<size_t>& idx) { return data_[flatten(idx)]; }
    const double& operator()(const std::vector<size_t>& idx) const { return data_[flatten(idx)]; }
    std::vector<double>& data() { return data_; }
    const std::vector<double>& data() const { return data_; }
private:
    std::vector<size_t> shape_;
    std::vector<double> data_;
    size_t flatten(const std::vector<size_t>& idx) const {
        if (idx.size() != shape_.size())
            throw std::invalid_argument("Tensor: index rank mismatch");
        size_t flat = 0, stride = 1;
        for (int i = shape_.size() - 1; i >= 0; --i) {
            if (idx[i] >= shape_[i])
                throw std::out_of_range("Tensor: index out of bounds");
            flat += idx[i] * stride;
            stride *= shape_[i];
        }
        return flat;
    }
};

} // namespace mytrix
