#include "boost_tree.h"
#include "decision_tree.h"
#include <numeric>
#include <cmath>
#include <algorithm>

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
        // Compute residuals
        std::vector<double> residuals(n);
        for (size_t i = 0; i < n; ++i)
            residuals[i] = y[i] - predictions[i];

        // Convert doubles to int for DecisionTree (quantize residuals)
        // Use a simple regression tree approach: discretize residuals into buckets
        // For simplicity, train a tree on quantized features predicting sign of residual
        std::vector<std::vector<int>> X_int(n);
        std::vector<int> y_int(n);
        for (size_t i = 0; i < n; ++i) {
            X_int[i].resize(X[i].size());
            for (size_t j = 0; j < X[i].size(); ++j)
                X_int[i][j] = static_cast<int>(X[i][j] * 100); // quantize
            y_int[i] = residuals[i] >= 0 ? 1 : 0;
        }

        auto tree = std::make_unique<DecisionTree>(SplitCriterion::GINI);
        tree->fit(X_int, y_int, static_cast<int>(params_.max_depth));

        // Update predictions using learning rate
        for (size_t i = 0; i < n; ++i) {
            int pred = tree->predict(X_int[i]);
            double update = (pred == 1) ? std::abs(residuals[i]) : -std::abs(residuals[i]);
            predictions[i] += params_.learning_rate * update;
        }

        estimators_.push_back(std::move(tree));
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

    std::vector<int> sample_int(sample.size());
    for (size_t j = 0; j < sample.size(); ++j)
        sample_int[j] = static_cast<int>(sample[j] * 100);

    for (const auto& tree : estimators_) {
        int pred = tree->predict(sample_int);
        prediction += params_.learning_rate * (pred == 1 ? 1.0 : -1.0);
    }
    return prediction;
}
