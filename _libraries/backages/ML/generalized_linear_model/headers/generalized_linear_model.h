#ifndef GLM_H
#define GLM_H

#include <vector>
#include <stdexcept>
#include <iostream>
#include <numeric>
#include <cmath>
#include <string>

class FitMethod {
public:
    virtual ~FitMethod() = default;
};

// Abstract base class for Generalized Linear Models
class GLM {
public:
    // Constructor takes a reference to a FitMethod, which is then owned by the GLM instance.
    GLM(const FitMethod& fit_method) : fit_method_(fit_method) {};

    // Virtual destructor to ensure derived class destructors are called correctly.
    virtual ~GLM() = default;

    /**
     * @brief Trains the model using the provided dataset. This is a pure virtual function.
     * @param X The feature matrix.
     * @param y The target vector.
     */
    virtual void fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) = 0;

    /**
     * @brief Predicts the output for a single sample. This is a pure virtual function.
     * @param sample The feature vector for the sample.
     * @return The predicted value.
     */
    virtual double predict(const std::vector<double>& sample) const = 0;

protected:
    std::vector<double> weights_;
    double bias_;
    const FitMethod& fit_method_;

    // Common utility methods that can be shared by derived classes
    void initialize_parameters(int num_features);
    
    // Abstract methods for derived classes to implement
    virtual double link_function(double linear_combination) const = 0;
    virtual double inverse_link_function(double predicted_value) const = 0;
    virtual double cost_function_derivative(double predicted_y, double actual_y) const = 0;
};

#endif // GLM_H
