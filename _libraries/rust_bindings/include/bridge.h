#pragma once

#include "rust/cxx.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace coolbox::rust_bindings {

class LinearRegressionModel {
public:
    LinearRegressionModel(std::vector<double> weights,
                          double intercept,
                          std::string method);

    rust::Vec<double> weights() const;
    double intercept() const;
    std::size_t feature_count() const;
    rust::String method_name() const;

private:
    std::vector<double> weights_;
    double intercept_;
    std::string method_;
};

std::unique_ptr<LinearRegressionModel> fit_linear_regression(
    rust::Slice<const double> x_flat,
    std::size_t rows,
    std::size_t cols,
    rust::Slice<const double> y,
    rust::Str method,
    std::uint32_t iterations,
    double learning_rate);

rust::Vec<double> predict_linear_regression(
    const LinearRegressionModel& model,
    rust::Slice<const double> x_flat,
    std::size_t rows,
    std::size_t cols);

} // namespace coolbox::rust_bindings
