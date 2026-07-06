#include "boost_tree.h"
#include <numeric>
#include <cmath>
#include <algorithm>
#include <limits>

BoostTree::BoostTree(const BoostTreeParameters& params) : params_(params) {}

BoostTree::~BoostTree() = default;

void BoostTree::fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
    estimators_.clear();
    size_t n = y.size();
    if (n == 0) return;

    // Initial prediction = mean of y
    initial_prediction_ = std::accumulate(y.begin(), y.end(), 0.0) / n;

    // Current predictions
    std::vector<double> predictions(n, initial_prediction_);

    for (unsigned int iter = 0; iter < params_.num_estimators; ++iter) {
        std::vector<double> residuals(n);
        for (size_t i = 0; i < n; ++i) {
            residuals[i] = y[i] - predictions[i];
        }

        WeakLearner best_learner;
        double best_loss = std::numeric_limits<double>::infinity();

        for (size_t feature_index = 0; feature_index < X[0].size(); ++feature_index) {
            std::vector<std::pair<double, double>> feature_values;
            feature_values.reserve(n);
            for (size_t sample_index = 0; sample_index < n; ++sample_index) {
                feature_values.emplace_back(X[sample_index][feature_index], residuals[sample_index]);
            }

            std::sort(feature_values.begin(), feature_values.end(),
                [](const auto& left, const auto& right) {
                    return left.first < right.first;
                });

            double total_sum = 0.0;
            double total_square_sum = 0.0;
            for (const auto& feature_value : feature_values) {
                total_sum += feature_value.second;
                total_square_sum += feature_value.second * feature_value.second;
            }

            double left_sum = 0.0;
            double left_square_sum = 0.0;
            size_t left_count = 0;

            for (size_t split_index = 0; split_index + 1 < feature_values.size(); ++split_index) {
                const double residual = feature_values[split_index].second;
                left_sum += residual;
                left_square_sum += residual * residual;
                ++left_count;

                if (feature_values[split_index].first == feature_values[split_index + 1].first) {
                    continue;
                }

                const size_t right_count = n - left_count;
                const double right_sum = total_sum - left_sum;
                const double right_square_sum = total_square_sum - left_square_sum;
                const double left_mean = left_sum / static_cast<double>(left_count);
                const double right_mean = right_sum / static_cast<double>(right_count);
                const double left_loss = left_square_sum - left_sum * left_mean;
                const double right_loss = right_square_sum - right_sum * right_mean;
                const double total_loss = left_loss + right_loss;

                if (total_loss < best_loss) {
                    best_loss = total_loss;
                    best_learner.feature_index = feature_index;
                    best_learner.threshold =
                        (feature_values[split_index].first + feature_values[split_index + 1].first) / 2.0;
                    best_learner.left_value = left_mean;
                    best_learner.right_value = right_mean;
                    best_learner.has_split = true;
                }
            }
        }

        if (!best_learner.has_split) {
            const double residual_mean = std::accumulate(residuals.begin(), residuals.end(), 0.0)
                / static_cast<double>(n);
            best_learner.left_value = residual_mean;
            best_learner.right_value = residual_mean;
        }

        for (size_t i = 0; i < n; ++i) {
            const double feature_value = X[i][best_learner.feature_index];
            const double update = (!best_learner.has_split || feature_value <= best_learner.threshold)
                ? best_learner.left_value
                : best_learner.right_value;
            predictions[i] += params_.learning_rate * update;
        }

        estimators_.push_back(best_learner);
    }
}

double BoostTree::predict(const std::vector<double>& sample) const {
    return predict_single(sample);
}

std::vector<double> BoostTree::predict(const std::vector<std::vector<double>>& X) const {
    std::vector<double> results;
    results.reserve(X.size());
    for (const auto& sample : X)
        results.push_back(predict_single(sample));
    return results;
}

double BoostTree::predict_single(const std::vector<double>& sample) const {
    double prediction = initial_prediction_;

    for (const auto& learner : estimators_) {
        const double feature_value = sample[learner.feature_index];
        const double update = (!learner.has_split || feature_value <= learner.threshold)
            ? learner.left_value
            : learner.right_value;
        prediction += params_.learning_rate * update;
    }

    return prediction;
}
