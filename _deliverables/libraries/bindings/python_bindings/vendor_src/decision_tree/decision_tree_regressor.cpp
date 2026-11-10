#include "decision_tree_regressor.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace {

// Welford's update avoids subtracting large, nearly equal sums of squares.
struct TargetStats {
    std::size_t count = 0;
    long double mean = 0.0L;
    long double squared_error = 0.0L;

    void add(double value) {
        ++count;
        const long double delta = static_cast<long double>(value) - mean;
        mean += delta / count;
        squared_error += delta * (static_cast<long double>(value) - mean);
    }
};

} // namespace

struct DecisionTreeRegressor::RegressionNode {
    double value = 0.0;
    std::size_t feature_index = 0;
    double threshold = 0.0;
    std::unique_ptr<RegressionNode> left;
    std::unique_ptr<RegressionNode> right;
};

DecisionTreeRegressor::DecisionTreeRegressor() = default;
DecisionTreeRegressor::~DecisionTreeRegressor() = default;

void DecisionTreeRegressor::fit(const std::vector<std::vector<double>>& X,
                                const std::vector<double>& y, int max_depth) {
    if (X.empty() || X.size() != y.size() || X.front().empty() || max_depth < 0) {
        throw std::invalid_argument("Regression requires nonempty matching data, features, and nonnegative max_depth");
    }
    const std::size_t num_features = X.front().size();
    for (std::size_t i = 0; i < X.size(); ++i) {
        if (X[i].size() != num_features || !std::isfinite(y[i])) {
            throw std::invalid_argument("Regression requires rectangular features and finite targets");
        }
        for (double value : X[i]) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument("Regression features must be finite");
            }
        }
    }

    std::vector<std::size_t> indices(X.size());
    std::iota(indices.begin(), indices.end(), 0);
    auto new_root = build_tree(X, y, indices, max_depth);
    root_ = std::move(new_root);
    num_features_ = num_features;
}

double DecisionTreeRegressor::predict(const std::vector<double>& sample) const {
    if (!root_) {
        throw std::logic_error("DecisionTreeRegressor must be fitted before prediction");
    }
    if (sample.size() != num_features_) {
        throw std::invalid_argument("Sample feature count must match the training data");
    }
    for (double value : sample) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("Sample features must be finite");
        }
    }
    const RegressionNode* node = root_.get();
    while (node->left) {
        node = sample[node->feature_index] <= node->threshold
            ? node->left.get() : node->right.get();
    }
    return node->value;
}

std::unique_ptr<DecisionTreeRegressor::RegressionNode> DecisionTreeRegressor::build_tree(
    const std::vector<std::vector<double>>& X, const std::vector<double>& y,
    const std::vector<std::size_t>& indices, int remaining_depth) {
    auto node = std::make_unique<RegressionNode>();
    TargetStats stats;
    bool constant = true;
    for (std::size_t index : indices) {
        stats.add(y[index]);
        constant = constant && y[index] == y[indices.front()];
    }
    node->value = static_cast<double>(stats.mean);
    if (!std::isfinite(node->value) || !std::isfinite(stats.squared_error)) {
        throw std::overflow_error("Regression target statistics exceed the numeric range");
    }
    if (remaining_depth == 0 || constant) {
        return node;
    }

    long double best_loss = std::numeric_limits<long double>::infinity();
    bool found_split = false;
    for (std::size_t feature = 0; feature < X.front().size(); ++feature) {
        auto sorted = indices;
        std::stable_sort(sorted.begin(), sorted.end(), [&](std::size_t a, std::size_t b) {
            return X[a][feature] < X[b][feature];
        });
        std::vector<long double> right_errors(sorted.size());
        TargetStats right;
        for (std::size_t i = sorted.size(); i > 0; --i) {
            right.add(y[sorted[i - 1]]);
            right_errors[i - 1] = right.squared_error;
        }
        TargetStats left;
        for (std::size_t i = 0; i + 1 < sorted.size(); ++i) {
            left.add(y[sorted[i]]);
            const double lower = X[sorted[i]][feature];
            const double upper = X[sorted[i + 1]][feature];
            if (lower == upper) {
                continue;
            }
            const long double loss = left.squared_error + right_errors[i + 1];
            if (!std::isfinite(loss)) {
                throw std::overflow_error("Regression split loss exceeds the numeric range");
            }
            if (loss < best_loss) {
                best_loss = loss;
                found_split = true;
                node->feature_index = feature;
                // Halving first avoids overflow; rounding must not empty a child.
                const double midpoint = lower / 2.0 + upper / 2.0;
                node->threshold = midpoint < upper ? std::max(lower, midpoint) : lower;
            }
        }
    }
    if (!found_split) {
        return node;
    }

    std::vector<std::size_t> left_indices;
    std::vector<std::size_t> right_indices;
    for (std::size_t index : indices) {
        if (X[index][node->feature_index] <= node->threshold) {
            left_indices.push_back(index);
        } else {
            right_indices.push_back(index);
        }
    }
    node->left = build_tree(X, y, left_indices, remaining_depth - 1);
    node->right = build_tree(X, y, right_indices, remaining_depth - 1);
    return node;
}
