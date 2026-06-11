#ifndef MARS_REGRESSION_H
#define MARS_REGRESSION_H

#include "generalized_linear_model.h"

#include <vector>

class MarsRegressionFitMethod : public FitMethod {
public:
    MarsRegressionFitMethod(
        int max_terms = 8,
        int min_samples_per_knot = 3,
        int max_knots_per_feature = 16,
        double ridge_lambda = 1e-8)
        : max_terms_(max_terms),
          min_samples_per_knot_(min_samples_per_knot),
          max_knots_per_feature_(max_knots_per_feature),
          ridge_lambda_(ridge_lambda) {}

    int get_max_terms() const { return max_terms_; }
    int get_min_samples_per_knot() const { return min_samples_per_knot_; }
    int get_max_knots_per_feature() const { return max_knots_per_feature_; }
    double get_ridge_lambda() const { return ridge_lambda_; }

private:
    int max_terms_;
    int min_samples_per_knot_;
    int max_knots_per_feature_;
    double ridge_lambda_;
};

class MarsRegression : public GLM {
public:
    struct BasisTerm {
        int feature_index;
        double knot;
        int direction;

        double evaluate(const std::vector<double>& sample) const;
    };

    explicit MarsRegression(const MarsRegressionFitMethod& fit_method);

    void fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) override;
    double predict(const std::vector<double>& sample) const override;

    std::vector<double> predict_batch(const std::vector<std::vector<double>>& X) const;
    int num_terms() const;
    std::vector<BasisTerm> get_terms() const;

protected:
    double link_function(double linear_combination) const override;
    double inverse_link_function(double predicted_value) const override;
    double cost_function_derivative(double predicted_y, double actual_y) const override;

private:
    MarsRegressionFitMethod fit_method_copy_;
    std::vector<BasisTerm> terms_;
    std::vector<double> coefficients_;
    int num_features_;
    bool fitted_;

    std::vector<double> candidate_knots(
        const std::vector<std::vector<double>>& X,
        int feature_index,
        int max_knots,
        int min_samples_per_knot) const;

    std::vector<double> solve_coefficients(
        const std::vector<std::vector<double>>& X,
        const std::vector<double>& y,
        const std::vector<BasisTerm>& candidate_terms,
        double ridge_lambda) const;

    double compute_rss(
        const std::vector<std::vector<double>>& X,
        const std::vector<double>& y,
        const std::vector<BasisTerm>& candidate_terms,
        const std::vector<double>& coeffs) const;
};

#endif // MARS_REGRESSION_H
