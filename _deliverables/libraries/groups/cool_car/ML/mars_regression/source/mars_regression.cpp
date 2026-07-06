#include "mars_regression.h"

#include "matrix_dense.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

constexpr double kSmall = 1e-12;

bool valid_sample_shape(const std::vector<std::vector<double>>& X) {
    if (X.empty()) {
        return false;
    }

    const std::size_t width = X[0].size();
    if (width == 0) {
        return false;
    }

    for (const auto& row : X) {
        if (row.size() != width) {
            return false;
        }
    }

    return true;
}

double median_of_vector(std::vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());
    const std::size_t mid = values.size() / 2;
    if ((values.size() % 2) == 0) {
        return 0.5 * (values[mid - 1] + values[mid]);
    }
    return values[mid];
}

} // namespace

double MarsRegression::BasisTerm::evaluate(const std::vector<double>& sample) const {
    const double delta = sample[feature_index] - knot;
    if (direction >= 0) {
        return std::max(0.0, delta);
    }
    return std::max(0.0, -delta);
}

MarsRegression::MarsRegression(const MarsRegressionFitMethod& fit_method)
    : GLM(fit_method),
      fit_method_copy_(fit_method),
      num_features_(0),
      fitted_(false) {}

void MarsRegression::fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
    if (!valid_sample_shape(X) || y.empty() || X.size() != y.size()) {
        throw std::invalid_argument("Invalid input dimensions for MarsRegression::fit");
    }

    const int n_samples = static_cast<int>(X.size());
    num_features_ = static_cast<int>(X[0].size());
    const int max_terms = std::max(0, fit_method_copy_.get_max_terms());

    if (n_samples < 2) {
        throw std::invalid_argument("MarsRegression requires at least two samples");
    }

    terms_.clear();
    coefficients_.clear();

    std::vector<BasisTerm> best_terms;
    std::vector<double> best_coefficients = solve_coefficients(X, y, best_terms, fit_method_copy_.get_ridge_lambda());
    double best_rss = compute_rss(X, y, best_terms, best_coefficients);

    for (int term_index = 0; term_index < max_terms; ++term_index) {
        bool improved = false;
        std::vector<BasisTerm> iteration_terms = best_terms;
        std::vector<double> iteration_coefficients = best_coefficients;
        double iteration_rss = best_rss;

        for (int feature = 0; feature < num_features_; ++feature) {
            const std::vector<double> knots = candidate_knots(
                X,
                feature,
                fit_method_copy_.get_max_knots_per_feature(),
                fit_method_copy_.get_min_samples_per_knot());

            for (double knot : knots) {
                for (int direction : {1, -1}) {
                    std::vector<BasisTerm> candidate_terms = best_terms;
                    candidate_terms.push_back(BasisTerm{feature, knot, direction});

                    const std::vector<double> candidate_coefficients = solve_coefficients(
                        X,
                        y,
                        candidate_terms,
                        fit_method_copy_.get_ridge_lambda());
                    const double candidate_rss = compute_rss(X, y, candidate_terms, candidate_coefficients);

                    if (candidate_rss + kSmall < iteration_rss) {
                        iteration_rss = candidate_rss;
                        iteration_terms = candidate_terms;
                        iteration_coefficients = candidate_coefficients;
                        improved = true;
                    }
                }
            }
        }

        if (!improved) {
            break;
        }

        best_terms = iteration_terms;
        best_coefficients = iteration_coefficients;
        best_rss = iteration_rss;
    }

    terms_ = best_terms;
    coefficients_ = best_coefficients;

    bias_ = coefficients_.empty() ? 0.0 : coefficients_[0];
    weights_.assign(terms_.size(), 0.0);
    for (std::size_t i = 0; i < terms_.size(); ++i) {
        weights_[i] = coefficients_[i + 1];
    }

    fitted_ = true;
}

double MarsRegression::predict(const std::vector<double>& sample) const {
    if (!fitted_) {
        throw std::runtime_error("MarsRegression model must be fitted before predict");
    }
    if (static_cast<int>(sample.size()) != num_features_) {
        throw std::invalid_argument("MarsRegression::predict sample size mismatch");
    }

    double value = coefficients_[0];
    for (std::size_t i = 0; i < terms_.size(); ++i) {
        value += coefficients_[i + 1] * terms_[i].evaluate(sample);
    }
    return inverse_link_function(value);
}

std::vector<double> MarsRegression::predict_batch(const std::vector<std::vector<double>>& X) const {
    std::vector<double> predictions;
    predictions.reserve(X.size());
    for (const auto& sample : X) {
        predictions.push_back(predict(sample));
    }
    return predictions;
}

int MarsRegression::num_terms() const {
    return static_cast<int>(terms_.size());
}

std::vector<MarsRegression::BasisTerm> MarsRegression::get_terms() const {
    return terms_;
}

double MarsRegression::link_function(double linear_combination) const {
    return linear_combination;
}

double MarsRegression::inverse_link_function(double predicted_value) const {
    return predicted_value;
}

double MarsRegression::cost_function_derivative(double predicted_y, double actual_y) const {
    return predicted_y - actual_y;
}

std::vector<double> MarsRegression::candidate_knots(
    const std::vector<std::vector<double>>& X,
    int feature_index,
    int max_knots,
    int min_samples_per_knot) const
{
    std::vector<double> values;
    values.reserve(X.size());
    for (const auto& sample : X) {
        values.push_back(sample[feature_index]);
    }

    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());

    if (values.size() <= 2) {
        return values;
    }

    std::vector<double> knots;
    const int min_index = std::max(1, min_samples_per_knot);
    const int max_index = std::max(min_index, static_cast<int>(values.size()) - min_samples_per_knot - 1);

    for (int i = min_index; i <= max_index; ++i) {
        knots.push_back(values[static_cast<std::size_t>(i)]);
    }

    if (knots.empty()) {
        knots.push_back(median_of_vector(values));
    }

    if (max_knots > 0 && static_cast<int>(knots.size()) > max_knots) {
        std::vector<double> reduced;
        reduced.reserve(static_cast<std::size_t>(max_knots));
        for (int i = 0; i < max_knots; ++i) {
            const double alpha = (max_knots == 1) ? 0.0 : static_cast<double>(i) / static_cast<double>(max_knots - 1);
            const std::size_t pos = static_cast<std::size_t>(alpha * static_cast<double>(knots.size() - 1));
            reduced.push_back(knots[pos]);
        }
        std::sort(reduced.begin(), reduced.end());
        reduced.erase(std::unique(reduced.begin(), reduced.end()), reduced.end());
        return reduced;
    }

    return knots;
}

std::vector<double> MarsRegression::solve_coefficients(
    const std::vector<std::vector<double>>& X,
    const std::vector<double>& y,
    const std::vector<BasisTerm>& candidate_terms,
    double ridge_lambda) const
{
    const int n_samples = static_cast<int>(X.size());
    const int n_columns = static_cast<int>(candidate_terms.size()) + 1;

    mytrix::DenseMatrix design(n_samples, n_columns);
    for (int i = 0; i < n_samples; ++i) {
        design.at(i, 0) = 1.0;
        for (int j = 0; j < static_cast<int>(candidate_terms.size()); ++j) {
            design.at(i, j + 1) = candidate_terms[static_cast<std::size_t>(j)].evaluate(X[static_cast<std::size_t>(i)]);
        }
    }

    Eigen::VectorXd y_vector(n_samples);
    for (int i = 0; i < n_samples; ++i) {
        y_vector(i) = y[static_cast<std::size_t>(i)];
    }

    const Eigen::MatrixXd xtx = design.data.transpose() * design.data;
    Eigen::MatrixXd regularized = xtx;
    if (ridge_lambda > 0.0) {
        regularized += ridge_lambda * Eigen::MatrixXd::Identity(n_columns, n_columns);
    }
    const Eigen::VectorXd xty = design.data.transpose() * y_vector;

    Eigen::VectorXd coefficients = regularized.ldlt().solve(xty);
    if ((regularized * coefficients).isApprox(xty, 1e-7) == false) {
        coefficients = regularized.completeOrthogonalDecomposition().solve(xty);
    }

    std::vector<double> result(static_cast<std::size_t>(n_columns), 0.0);
    for (int i = 0; i < n_columns; ++i) {
        result[static_cast<std::size_t>(i)] = coefficients(i);
    }
    return result;
}

double MarsRegression::compute_rss(
    const std::vector<std::vector<double>>& X,
    const std::vector<double>& y,
    const std::vector<BasisTerm>& candidate_terms,
    const std::vector<double>& coeffs) const
{
    double rss = 0.0;
    for (std::size_t i = 0; i < X.size(); ++i) {
        double prediction = coeffs[0];
        for (std::size_t j = 0; j < candidate_terms.size(); ++j) {
            prediction += coeffs[j + 1] * candidate_terms[j].evaluate(X[i]);
        }
        const double error = prediction - y[i];
        rss += error * error;
    }
    return rss;
}
