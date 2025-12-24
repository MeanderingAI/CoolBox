
#include <iostream>
#include <stdexcept>
#include <random>
#include <Eigen/Dense>
#include "ml/generalized_linear_model/linear_regression.h"
#include "ml/generalized_linear_model/generalized_linear_model.h"


/**
 * @brief The identity link function for linear regression.
 * @param linear_combination The result of the dot product of weights and features plus bias.
 * @return The linear combination itself.
 */
double LinearRegression::link_function(double linear_combination) const {
    return linear_combination;
}

/**
 * @brief The inverse identity link function.
 * @param predicted_value The predicted value.
 * @return The predicted value itself.
 */
double LinearRegression::inverse_link_function(double predicted_value) const {
    return predicted_value;
}

/**
 * @brief The derivative of the Mean Squared Error cost function.
 * @param predicted_y The predicted value.
 * @param actual_y The actual value.
 * @return The derivative of the cost with respect to the prediction.
 */
double LinearRegression::cost_function_derivative(double predicted_y, double actual_y) const {
    return predicted_y - actual_y;
}
/**
 * @brief Trains the linear regression model using a closed-form solution (Normal Equation).
 * @param X The feature matrix.
 * @param y The target vector.
 */
void LinearRegression::fit_closed_form(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
    if (X.empty() || y.empty() || X.size() != y.size()) {
        throw std::invalid_argument("Input data is invalid or has a size mismatch.");
    }
    
    int num_samples = X.size();
    if (num_samples > 0 && X[0].empty()) {
        throw std::invalid_argument("Input feature vectors cannot be empty.");
    }
    int num_features = X[0].size();
    
    // Convert std::vectors to Eigen matrices
    Eigen::MatrixXd eigen_X(num_samples, num_features);
    Eigen::VectorXd eigen_y(num_samples);
    
    for (int i = 0; i < num_samples; ++i) {
        for (int j = 0; j < num_features; ++j) {
            eigen_X(i, j) = X[i][j];
        }
        eigen_y(i) = y[i];
    }
    
    // Add bias term to the feature matrix for a unified calculation
    Eigen::MatrixXd X_with_bias(num_samples, num_features + 1);
    X_with_bias.leftCols(num_features) = eigen_X;
    X_with_bias.col(num_features) = Eigen::VectorXd::Ones(num_samples);

    // Calculate coefficients using the normal equation: (X^T * X)^-1 * X^T * y
    Eigen::MatrixXd X_T = X_with_bias.transpose();
    Eigen::MatrixXd X_T_X = X_T * X_with_bias;
    Eigen::MatrixXd X_T_y = X_T * eigen_y;
    
    // Check for non-invertible matrix
    if (X_T_X.determinant() == 0) {
        throw std::runtime_error("Matrix X^T * X is non-invertible. Cannot use closed-form solution.");
    }

    Eigen::VectorXd coefficients = X_T_X.inverse() * X_T_y;

    // Set the learned weights and bias
    weights_.assign(coefficients.data(), coefficients.data() + num_features);
    bias_ = coefficients(num_features);
}

/**
 * @brief Trains the linear regression model using Stochastic Gradient Descent (SGD).
 * @param X The feature matrix.
 * @param y The target vector.
 */
void LinearRegression::fit_sgd(const std::vector<std::vector<double>>& X, const std::vector<double>& y,  const LinearRegressionFitMethod& fit_method) {
    if (X.empty() || y.empty() || X.size() != y.size()) {
        throw std::invalid_argument("Input data is invalid or has a size mismatch.");
    }

    int num_samples = X.size();
    if (num_samples > 0 && X[0].empty()) {
        throw std::invalid_argument("Input feature vectors cannot be empty.");
    }

    int num_features = X[0].size();
    double learning_rate = fit_method.get_learning_rate();
    int iterations = fit_method.get_num_iterations();

    
    // Perform SGD
    for (int i = 0; i < iterations; ++i) {
        // Shuffle the data for each epoch
        std::vector<int> indices(num_samples);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), g);
        
        for (int idx : indices) {
            double linear_combination = bias_;
            for (int k = 0; k < num_features; ++k) {
                linear_combination += X[idx][k] * weights_[k];
            }
            
            double predicted_y = link_function(linear_combination);
            double error = cost_function_derivative(predicted_y, y[idx]);
            
            // Update weights and bias using only the current sample
            bias_ -= learning_rate * error;
            for (int k = 0; k < num_features; ++k) {
                weights_[k] -= learning_rate * error * X[idx][k];
            }
        }
    }
}

/**
 * @brief Trains the model using the specified fitting method.
 * @param X The feature matrix.
 * @param y The target vector.
 * @param method The fitting method to use.
 */
void LinearRegression::fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
    const auto& lr_fit_method = static_cast<const LinearRegressionFitMethod&>(fit_method_);

    const auto& method = lr_fit_method.get_type();
    switch (method) {
        case LinearRegressionFitMethod::Type::GRADIENT_DESCENT:
            fit_sgd(X, y, lr_fit_method);
            break;
        case LinearRegressionFitMethod::Type::CLOSED_FORM:
            fit_closed_form(X, y);
            break;
        default:
            throw std::invalid_argument("Invalid fitting method specified.");
    }
}

/**
 * @brief Predicts the output for a single sample.
 * @param sample The feature vector for the sample to predict.
 * @return The predicted value.
 */
double LinearRegression::predict(const std::vector<double>& sample) const {
    if (sample.size() != weights_.size()) {
        throw std::invalid_argument("Sample feature size does not match model's feature size.");
    }

    double linear_combination = bias_;
    for (size_t i = 0; i < sample.size(); ++i) {
        linear_combination += sample[i] * weights_[i];
    }
    
    // Use the link function to get the final prediction
    return link_function(linear_combination);
}

/**
 * @brief Retrieves the learned coefficients.
 * @return A pair containing the vector of weights and the bias.
 */
std::pair<std::vector<double>, double> LinearRegression::get_coefficients() const {
    return {weights_, bias_};
}