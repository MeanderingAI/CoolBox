// xarray.h
// Minimal N-dimensional array (xarray) implementation for DATASTRUCTURE package
// Author: [Your Name]
// License: Same as project
#pragma once

#include <vector>
#include <array>
#include <cassert>
#include <numeric>
#include <initializer_list>
#include <stdexcept>

// A simple N-dimensional array (xarray) implementation
// Similar in spirit to Python's xarray or NumPy ndarray

template <typename T, size_t N>
class xarray {
public:
    using shape_type = std::array<size_t, N>;
    using index_type = std::array<size_t, N>;

    xarray(const shape_type& shape)
        : shape_(shape), data_(compute_size(shape)) {}

    xarray(const shape_type& shape, const T& value)
        : shape_(shape), data_(compute_size(shape), value) {}

    xarray(std::initializer_list<T> init) : shape_{init.size()}, data_(init) {
        static_assert(N == 1, "Initializer list only supported for 1D arrays");
    }

    T& operator[](const index_type& idx) {
        return data_[flatten_index(idx)];
    }
    const T& operator[](const index_type& idx) const {
        return data_[flatten_index(idx)];
    }

    const shape_type& shape() const { return shape_; }
    size_t size() const { return data_.size(); }

private:
    shape_type shape_;
    std::vector<T> data_;

    static size_t compute_size(const shape_type& shape) {
        return std::accumulate(shape.begin(), shape.end(), 1ull, std::multiplies<>());
    }

    size_t flatten_index(const index_type& idx) const {
        size_t flat = 0;
        size_t stride = 1;
        for (size_t i = N; i-- > 0;) {
            if (idx[i] >= shape_[i]) throw std::out_of_range("xarray: index out of bounds");
            flat += idx[i] * stride;
            stride *= shape_[i];
        }
        return flat;
    }
};
