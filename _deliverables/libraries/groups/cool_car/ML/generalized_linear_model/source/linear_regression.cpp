#include "linear_regression.h"
#include <numeric>
#include <cmath>
#include <algorithm>

void LinearRegression::fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
    if (X.empty() || y.empty() || X.size() != y.size()) {
        throw std::invalid_argument("Invalid input dimensions for fit.");
    }

    int num_samples = static_cast<int>(X.size());
    int num_features = static_cast<int>(X[0].size());
    initialize_parameters(num_features);

    const auto& method = fit_method_copy_;

    if (method.get_type() == LinearRegressionFitMethod::CLOSED_FORM) {
        // Normal equation: w = (X^T X)^{-1} X^T y
        // Simple implementation using direct computation
        // Add bias column
        std::vector<std::vector<double>> X_aug(num_samples, std::vector<double>(num_features + 1));
        for (int i = 0; i < num_samples; ++i) {
            X_aug[i][0] = 1.0; // bias
            for (int j = 0; j < num_features; ++j) {
                X_aug[i][j + 1] = X[i][j];
            }
        }
        int cols = num_features + 1;
        // Compute X^T X
        std::vector<std::vector<double>> XtX(cols, std::vector<double>(cols, 0.0));
        for (int i = 0; i < cols; ++i) {
            for (int j = 0; j < cols; ++j) {
                for (int k = 0; k < num_samples; ++k) {
                    XtX[i][j] += X_aug[k][i] * X_aug[k][j];
                }
            }
        }
        // Compute X^T y
        std::vector<double> Xty(cols, 0.0);
        for (int i = 0; i < cols; ++i) {
            for (int k = 0; k < num_samples; ++k) {
                Xty[i] += X_aug[k][i] * y[k];
            }
        }
        // Solve using Gaussian elimination
        std::vector<std::vector<double>> aug(cols, std::vector<double>(cols + 1));
        for (int i = 0; i < cols; ++i) {
            for (int j = 0; j < cols; ++j) aug[i][j] = XtX[i][j];
            aug[i][cols] = Xty[i];
        }
        for (int i = 0; i < cols; ++i) {
            int max_row = i;
            for (int k = i + 1; k < cols; ++k) {
                if (std::abs(aug[k][i]) > std::abs(aug[max_row][i])) max_row = k;
            }
            std::swap(aug[i], aug[max_row]);
            if (std::abs(aug[i][i]) < 1e-12) continue;
            for (int k = i + 1; k < cols; ++k) {
                double factor = aug[k][i] / aug[i][i];
                for (int j = i; j <= cols; ++j) aug[k][j] -= factor * aug[i][j];
            }
        }
        std::vector<double> solution(cols, 0.0);
        for (int i = cols - 1; i >= 0; --i) {
            solution[i] = aug[i][cols];
            for (int j = i + 1; j < cols; ++j) {
                solution[i] -= aug[i][j] * solution[j];
            }
            if (std::abs(aug[i][i]) > 1e-12)
                solution[i] /= aug[i][i];
        }
        bias_ = solution[0];
        for (int j = 0; j < num_features; ++j) {
            weights_[j] = solution[j + 1];
        }
    } else {
        // Gradient descent
        unsigned int iterations = method.get_num_iterations();
        double lr = method.get_learning_rate();

        // Random initialization
        std::normal_distribution<double> dist(0.0, 0.01);
        for (auto& w : weights_) w = dist(g);
        bias_ = dist(g);

        for (unsigned int iter = 0; iter < iterations; ++iter) {
            std::vector<double> dw(num_features, 0.0);
            double db = 0.0;

            for (int i = 0; i < num_samples; ++i) {
                double pred = predict(X[i]);
                double error = cost_function_derivative(pred, y[i]);
                for (int j = 0; j < num_features; ++j) {
                    dw[j] += error * X[i][j];
                }
                db += error;
            }

            for (int j = 0; j < num_features; ++j) {
                weights_[j] -= lr * dw[j] / num_samples;
            }
            bias_ -= lr * db / num_samples;
        }
    }
}

double LinearRegression::predict(const std::vector<double>& sample) const {
    double linear_combination = bias_;
    for (size_t j = 0; j < sample.size() && j < weights_.size(); ++j) {
        linear_combination += weights_[j] * sample[j];
    }
    return inverse_link_function(linear_combination);
}

std::pair<std::vector<double>, double> LinearRegression::get_coefficients() const {
    return {weights_, bias_};
}

double LinearRegression::link_function(double linear_combination) const {
    return linear_combination; // identity
}

double LinearRegression::inverse_link_function(double predicted_value) const {
    return predicted_value; // identity
}

double LinearRegression::cost_function_derivative(double predicted_y, double actual_y) const {
    return predicted_y - actual_y; // MSE derivative
}
