#ifndef TENSOR_H
#define TENSOR_H

#include <vector>
#include <memory>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>

namespace ml {
namespace deep_learning {

class Tensor {
public:
    // Constructors
    Tensor() : data_(), shape_() {}
    
    Tensor(const std::vector<size_t>& shape);
    Tensor(const std::vector<size_t>& shape, double fill_value);
    Tensor(const std::vector<size_t>& shape, const std::vector<double>& data);
    
    // Getters
    const std::vector<double>& data() const { return data_; }
    std::vector<double>& data() { return data_; }
    const std::vector<size_t>& shape() const { return shape_; }
    size_t size() const { return data_.size(); }
    size_t ndim() const { return shape_.size(); }
    
    // Element access
    double& operator()(const std::vector<size_t>& indices);
    const double& operator()(const std::vector<size_t>& indices) const;
    double& at(size_t index) { return data_.at(index); }
    const double& at(size_t index) const { return data_.at(index); }
    
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
    
    // Utility
    void fill(double value);
    void randomize(double min = -1.0, double max = 1.0);
    Tensor clone() const;
    
