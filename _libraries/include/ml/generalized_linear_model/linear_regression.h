#ifndef LINEAR_REGRESSION_H
#define LINEAR_REGRESSION_H

#include <random>
#include "ML/generalized_linear_model/generalized_linear_model.h"

class LinearRegressionFitMethod: public FitMethod {
public:
    // Enum to represent the different methods for fitting a model.
    enum Type {
        GRADIENT_DESCENT,
        CLOSED_FORM
    };
    LinearRegressionFitMethod(unsigned int num_iterations = 10000,
        double learning_rate = 0.01, Type type = Type::GRADIENT_DESCENT)
        : num_iterations_(num_iterations), learning_rate_(learning_rate), type_(type) {}

    // Accessor methods to retrieve the private members.
    unsigned int get_num_iterations() const { return num_iterations_; }
    double get_learning_rate() const { return learning_rate_; }
    Type get_type() const { return type_; }
    
private:
    unsigned int num_iterations_;
    double learning_rate_;
    Type type_;
};

// Concrete class for Linear Regression, inheriting from GLM
class LinearRegression : public GLM {
private:
    std::mt19937 g;
public:
    // The constructor correctly initializes the base class first, then its own members.
    LinearRegression(const LinearRegressionFitMethod& fit_method) 
        : GLM(fit_method) {
        std::random_device rd;
        g.seed(rd());
    }

    /**
     * @brief Trains the model using the provided dataset.
     * @param X The feature matrix.
     * @param y The target vector.
     */
    void fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) override;

    /**
     * @brief Predicts the output for a single sample.
     * @param sample The feature vector for the sample.
     * @return The predicted value.
     */
    double predict(const std::vector<double>& sample) const override;

    /**
     * @brief Retrieves the learned coefficients (weights and bias).
     * @return A pair containing the vector of weights and the bias.
     */
    std::pair<std::vector<double>, double> get_coefficients() const;

protected:

    void fit_closed_form(const std::vector<std::vector<double>>& X, const std::vector<double>& y);
    void fit_sgd(const std::vector<std::vector<double>>& X, const std::vector<double>& y, const LinearRegressionFitMethod& fit_method);

    // Pure virtual methods from the base class are now implemented here.
    double link_function(double linear_combination) const override;
    double inverse_link_function(double predicted_value) const override;
    double cost_function_derivative(double predicted_y, double actual_y) const override;
};

#endif // LINEAR_REGRESSION_H
