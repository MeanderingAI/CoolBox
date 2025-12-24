#include <gtest/gtest.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#include "ml/generalized_linear_model/linear_regression.h"

// Helper function to check if two vectors are approximately equal
bool are_vectors_equal(const std::vector<double>& v1, const std::vector<double>& v2, double tolerance = 1e-6) {
    if (v1.size() != v2.size()) {
        return false;
    }
    for (size_t i = 0; i < v1.size(); ++i) {
        if (std::abs(v1[i] - v2[i]) > tolerance) {
            std::cout << "Vector mismatch at index " << i << ": v1=" << v1[i] << ", v2=" << v2[i] << std::endl;
            return false;
        }
    }
    return true;
}

// Helper function to check if two doubles are approximately equal
bool are_doubles_equal(double d1, double d2, double tolerance = 1e-6) {
    if (std::abs(d1 - d2) > tolerance) {
        std::cout << "Double mismatch: d1=" << d1 << ", d2=" << d2 << std::endl;
        return false;
    }
    return true;
}

TEST(LinearRegressionTest, ClosedFormSolution) {
    // Test the closed-form solution on a simple linear dataset.
    // Data follows the line y = 2x + 1
    std::vector<std::vector<double>> X = {{1.0}, {2.0}, {3.0}, {4.0}};
    std::vector<double> y = {3.0, 5.0, 7.0, 9.0};
    
    // The expected coefficients are the exact values from the line equation.
    std::vector<double> expected_weights = {2.0};
    double expected_bias = 1.0;

    // Create a fit method object for the CLOSED_FORM method.
    LinearRegressionFitMethod fit_method(0, 0, LinearRegressionFitMethod::Type::CLOSED_FORM);
    
    // Create and train the model.
    LinearRegression model(fit_method);
    model.fit(X, y);
    
    // Get the learned coefficients.
    auto coefficients = model.get_coefficients();
    
    // Assert that the learned coefficients match the expected ones.
    ASSERT_TRUE(are_vectors_equal(expected_weights, coefficients.first));
    ASSERT_TRUE(are_doubles_equal(expected_bias, coefficients.second));
}

TEST(LinearRegressionTest, GradientDescentSolution) {
    // Test the gradient descent solution on a simple linear dataset.
    // Data follows the line y = 2x + 1
    std::vector<std::vector<double>> X = {{1.0}, {2.0}, {3.0}, {4.0}};
    std::vector<double> y = {3.0, 5.0, 7.0, 9.0};

    // The expected coefficients are the exact values from the line equation.
    std::vector<double> expected_weights = {2.0};
    double expected_bias = 1.0;

    // Create a fit method object for the GRADIENT_DESCENT method.
    // Using default iterations and learning rate, which should be sufficient to get close.
    LinearRegressionFitMethod fit_method(10000, 0.01, LinearRegressionFitMethod::Type::GRADIENT_DESCENT);
    
    // Create and train the model.
    LinearRegression model(fit_method);
    model.fit(X, y);
    
    // Get the learned coefficients.
    auto coefficients = model.get_coefficients();
    
    // Assert that the learned coefficients are close to the expected ones.
    // We use a larger tolerance for gradient descent due to its iterative nature.
    ASSERT_TRUE(are_vectors_equal(expected_weights, coefficients.first, 1e-2));
    ASSERT_TRUE(are_doubles_equal(expected_bias, coefficients.second, 1e-2));
}

TEST(LinearRegressionTest, PredictionAccuracy) {
    // Test prediction on a trained model.
    std::vector<std::vector<double>> X = {{1.0}, {2.0}, {3.0}, {4.0}};
    std::vector<double> y = {3.0, 5.0, 7.0, 9.0};
    
    LinearRegressionFitMethod fit_method(10000, 0.01, LinearRegressionFitMethod::Type::GRADIENT_DESCENT);
    LinearRegression model(fit_method);
    model.fit(X, y);

    // Test a known sample from the training set.
    double predicted = model.predict({5.0});
    ASSERT_TRUE(are_doubles_equal(predicted, 11.0, 1e-2));
    
    // Test an unseen sample (interpolation).
    predicted = model.predict({2.5});
    ASSERT_TRUE(are_doubles_equal(predicted, 6.0, 1e-2));
}

TEST(LinearRegressionTest, ThrowsExceptionOnInvalidFitInput) {
    LinearRegressionFitMethod fit_method(100, 0.01, LinearRegressionFitMethod::Type::GRADIENT_DESCENT);
    LinearRegression model(fit_method);

    // Test with empty feature matrix and target vector
    std::vector<std::vector<double>> X_empty;
    std::vector<double> y_empty;
    EXPECT_THROW(model.fit(X_empty, y_empty), std::invalid_argument);

    // Test with size mismatch
    std::vector<std::vector<double>> X = {{1.0}, {2.0}};
    std::vector<double> y = {3.0, 5.0, 7.0};
    EXPECT_THROW(model.fit(X, y), std::invalid_argument);
}

TEST(LinearRegressionTest, ThrowsExceptionOnInvalidPredictionInput) {
    // First, train a model on a valid dataset.
    std::vector<std::vector<double>> X = {{1.0}, {2.0}};
    std::vector<double> y = {3.0, 5.0};
    LinearRegressionFitMethod fit_method(100, 0.01, LinearRegressionFitMethod::Type::GRADIENT_DESCENT);
    LinearRegression model(fit_method);
    model.fit(X, y);

    // Test with a sample that has the wrong number of features.
    std::vector<double> invalid_sample = {1.0, 2.0};
    EXPECT_THROW(model.predict(invalid_sample), std::invalid_argument);
}