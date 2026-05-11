#include "bridge.h"

#include "generalized_linear_model.h"
#include "linear_regression.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace coolbox::rust_bindings {
namespace {

std::vector<std::vector<double>> to_matrix(rust::Slice<const double> flat,
                                           std::size_t rows,
                                           std::size_t cols) {
    if (rows == 0 || cols == 0) {
        throw std::invalid_argument("feature matrix must be non-empty");
    }
    if (flat.size() != rows * cols) {
        throw std::invalid_argument("feature matrix shape does not match flattened data length");
    }

    std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols));
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < cols; ++j) {
            matrix[i][j] = flat[i * cols + j];
        }
    }
    return matrix;
}

std::vector<double> to_vector(rust::Slice<const double> values) {
    return std::vector<double>(values.begin(), values.end());
}

} // namespace

LinearRegressionModel::LinearRegressionModel(std::vector<double> weights,
                                             double intercept,
                                             std::string method)
    : weights_(std::move(weights)),
      intercept_(intercept),
      method_(std::move(method)) {}

rust::Vec<double> LinearRegressionModel::weights() const {
    rust::Vec<double> values;
    values.reserve(weights_.size());
    for (double weight : weights_) {
        values.push_back(weight);
    }
    return values;
}

double LinearRegressionModel::intercept() const {
    return intercept_;
}

std::size_t LinearRegressionModel::feature_count() const {
    return weights_.size();
}

rust::String LinearRegressionModel::method_name() const {
    return method_;
}

std::unique_ptr<LinearRegressionModel> fit_linear_regression(
    rust::Slice<const double> x_flat,
    std::size_t rows,
    std::size_t cols,
    rust::Slice<const double> y,
    rust::Str method,
    std::uint32_t iterations,
    double learning_rate) {
    if (y.size() != rows) {
        throw std::invalid_argument("target vector length must match the number of feature rows");
    }

    auto x = to_matrix(x_flat, rows, cols);
    auto y_vec = to_vector(y);
    std::string method_string(method);

    auto fit_type = method_string == "gradient_descent"
        ? LinearRegressionFitMethod::GRADIENT_DESCENT
        : LinearRegressionFitMethod::CLOSED_FORM;

    LinearRegressionFitMethod fit_method(iterations, learning_rate, fit_type);
    LinearRegression model(fit_method);
    model.fit(x, y_vec);

    auto [weights, intercept] = model.get_coefficients();
    return std::make_unique<LinearRegressionModel>(
        std::move(weights), intercept, std::move(method_string));
}

rust::Vec<double> predict_linear_regression(const LinearRegressionModel& model,
                                            rust::Slice<const double> x_flat,
                                            std::size_t rows,
                                            std::size_t cols) {
    if (cols != model.feature_count()) {
        throw std::invalid_argument("feature column count must match the fitted model");
    }

    auto x = to_matrix(x_flat, rows, cols);
    rust::Vec<double> predictions;
    predictions.reserve(rows);

    auto weights = model.weights();
    for (const auto& row : x) {
        double fitted = model.intercept();
        for (std::size_t j = 0; j < cols; ++j) {
            fitted += row[j] * weights[j];
        }
        predictions.push_back(fitted);
    }

    return predictions;
}

} // namespace coolbox::rust_bindings
