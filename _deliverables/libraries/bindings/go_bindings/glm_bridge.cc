#include "generalized_linear_model.h"
#include "linear_regression.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct LinearRegressionHandle {
    LinearRegressionFitMethod fit_method;
    LinearRegression model;
    bool fitted = false;

    LinearRegressionHandle(unsigned int iterations, double learning_rate, int fit_type)
        : fit_method(iterations, learning_rate,
                     fit_type == 1 ? LinearRegressionFitMethod::CLOSED_FORM
                                   : LinearRegressionFitMethod::GRADIENT_DESCENT),
          model(fit_method) {}
};

void write_error(char* buffer, size_t capacity, const char* message) {
    if (buffer == nullptr || capacity == 0) {
        return;
    }
    const size_t count = std::min(capacity - 1, std::strlen(message));
    std::memcpy(buffer, message, count);
    buffer[count] = '\0';
}

template <typename Function>
int invoke(char* error, size_t error_capacity, Function&& function) {
    try {
        function();
        return 1;
    } catch (const std::exception& exception) {
        write_error(error, error_capacity, exception.what());
    } catch (...) {
        write_error(error, error_capacity, "unknown native exception");
    }
    return 0;
}

LinearRegressionHandle& checked_handle(void* handle) {
    if (handle == nullptr) {
        throw std::invalid_argument("linear regression model is closed");
    }
    return *static_cast<LinearRegressionHandle*>(handle);
}

std::vector<std::vector<double>> make_matrix(const double* values,
                                             size_t rows,
                                             size_t cols) {
    if (rows == 0 || cols == 0 || values == nullptr) {
        throw std::invalid_argument("feature matrix must be non-empty");
    }
    if (rows > std::numeric_limits<size_t>::max() / cols) {
        throw std::invalid_argument("feature matrix dimensions overflow");
    }

    std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols));
    for (size_t row = 0; row < rows; ++row) {
        std::copy_n(values + row * cols, cols, matrix[row].begin());
    }
    return matrix;
}

} // namespace

extern "C" {

void* coolbox_glm_linear_regression_new(unsigned int iterations,
                                       double learning_rate,
                                       int fit_type) {
    if (fit_type != 0 && fit_type != 1) {
        return nullptr;
    }
    try {
        return new LinearRegressionHandle(iterations, learning_rate, fit_type);
    } catch (...) {
        return nullptr;
    }
}

void coolbox_glm_linear_regression_free(void* model) {
    delete static_cast<LinearRegressionHandle*>(model);
}

int coolbox_glm_linear_regression_fit(void* model,
                                     const double* x,
                                     size_t rows,
                                     size_t cols,
                                     const double* y,
                                     char* error,
                                     size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        auto& handle = checked_handle(model);
        if (y == nullptr || rows == 0) {
            throw std::invalid_argument("target vector must be non-empty");
        }
        auto matrix = make_matrix(x, rows, cols);
        handle.model.fit(matrix, std::vector<double>(y, y + rows));
        handle.fitted = true;
    });
}

int coolbox_glm_linear_regression_predict(void* model,
                                         const double* sample,
                                         size_t cols,
                                         double* prediction,
                                         char* error,
                                         size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        auto& handle = checked_handle(model);
        if (!handle.fitted) {
            throw std::invalid_argument("linear regression model has not been fitted");
        }
        if (sample == nullptr || prediction == nullptr || cols == 0) {
            throw std::invalid_argument("sample must be non-empty");
        }
        *prediction = handle.model.predict(std::vector<double>(sample, sample + cols));
    });
}

int coolbox_glm_linear_regression_coefficients(void* model,
                                               double* weights,
                                               size_t weight_capacity,
                                               double* intercept,
                                               char* error,
                                               size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        auto& handle = checked_handle(model);
        if (!handle.fitted) {
            throw std::invalid_argument("linear regression model has not been fitted");
        }
        if (intercept == nullptr) {
            throw std::invalid_argument("intercept output must not be null");
        }
        const auto coefficients = handle.model.get_coefficients();
        if (weights == nullptr || weight_capacity < coefficients.first.size()) {
            throw std::invalid_argument("weight output buffer is too small");
        }
        std::copy(coefficients.first.begin(), coefficients.first.end(), weights);
        *intercept = coefficients.second;
    });
}

size_t coolbox_glm_linear_regression_feature_count(void* model) {
    if (model == nullptr) {
        return 0;
    }
    auto& handle = *static_cast<LinearRegressionHandle*>(model);
    return handle.fitted ? handle.model.get_coefficients().first.size() : 0;
}

} // extern "C"
