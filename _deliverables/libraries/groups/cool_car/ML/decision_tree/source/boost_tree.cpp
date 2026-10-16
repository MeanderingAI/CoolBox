#include "boost_tree.h"
#include "tree_ensemble_utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <utility>

namespace {

void validate_parameters(const BoostTreeParameters& params) {
    if (params.num_estimators == 0 || !std::isfinite(params.learning_rate)
        || params.learning_rate <= 0.0
        || params.max_depth > static_cast<unsigned int>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Boosting requires positive estimators/rate and max_depth within int range");
    }
}

std::vector<double> softmax(const std::vector<double>& scores) {
    const double maximum = *std::max_element(scores.begin(), scores.end());
    std::vector<double> probabilities(scores.size());
    double total = 0.0;
    for (std::size_t k = 0; k < scores.size(); ++k) {
        probabilities[k] = std::exp(scores[k] - maximum);
        total += probabilities[k];
    }
    for (double& probability : probabilities) {
        probability /= total;
    }
    return probabilities;
}

double update_score(double score, double rate, double prediction) {
    return tree_ensemble_detail::checked_value(
        static_cast<long double>(score) + static_cast<long double>(rate) * prediction);
}

} // namespace

BoostTree::BoostTree(const BoostTreeParameters& params) : params_(params) {
    validate_parameters(params);
}

BoostTree::~BoostTree() = default;

void BoostTree::fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
    const auto features = tree_ensemble_detail::validate_training(X, y);
    long double mean = 0.0L;
    for (std::size_t i = 0; i < y.size(); ++i) {
        mean += (static_cast<long double>(y[i]) - mean) / (i + 1);
    }
    const double initial = tree_ensemble_detail::checked_value(mean);
    std::vector<double> predictions(y.size(), initial);
    std::vector<std::shared_ptr<const DecisionTreeRegressor>> estimators;
    estimators.reserve(params_.num_estimators);
    for (unsigned int round = 0; round < params_.num_estimators; ++round) {
        std::vector<double> residuals(y.size());
        for (std::size_t i = 0; i < y.size(); ++i) {
            residuals[i] = tree_ensemble_detail::checked_value(
                static_cast<long double>(y[i]) - predictions[i]);
        }
        auto tree = std::make_shared<DecisionTreeRegressor>();
        tree->fit(X, residuals, static_cast<int>(params_.max_depth));
        for (std::size_t i = 0; i < y.size(); ++i) {
            predictions[i] = update_score(predictions[i], params_.learning_rate, tree->predict(X[i]));
        }
        estimators.push_back(std::move(tree));
    }
    estimators_ = std::move(estimators);
    initial_prediction_ = initial;
    num_features_ = features;
}

double BoostTree::predict(const std::vector<double>& sample) const {
    return predict_single(sample);
}

std::vector<double> BoostTree::predict(const std::vector<std::vector<double>>& X) const {
    if (num_features_ == 0) {
        throw std::logic_error("Model must be fitted before prediction");
    }
    std::vector<double> results;
    results.reserve(X.size());
    for (const auto& sample : X) {
        results.push_back(predict_single(sample));
    }
    return results;
}

double BoostTree::predict_single(const std::vector<double>& sample) const {
    tree_ensemble_detail::validate_sample(sample, num_features_);
    double prediction = initial_prediction_;
    for (const auto& tree : estimators_) {
        prediction = update_score(prediction, params_.learning_rate, tree->predict(sample));
    }
    return prediction;
}

BoostTreeClassifier::BoostTreeClassifier(const BoostTreeParameters& params) : params_(params) {
    validate_parameters(params);
}

void BoostTreeClassifier::fit(const std::vector<std::vector<double>>& X, const std::vector<int>& y) {
    const auto features = tree_ensemble_detail::validate_training(X, y);
    std::map<int, std::size_t> counts;
    for (int label : y) {
        ++counts[label];
    }
    std::vector<int> labels;
    std::vector<double> initial;
    for (const auto& entry : counts) {
        labels.push_back(entry.first);
        initial.push_back(std::log(static_cast<double>(entry.second) / y.size()));
    }
    std::vector<std::size_t> encoded;
    for (int label : y) {
        encoded.push_back(std::lower_bound(labels.begin(), labels.end(), label) - labels.begin());
    }
    std::vector<std::vector<double>> scores(y.size(), initial);
    std::vector<std::vector<std::unique_ptr<DecisionTreeRegressor>>> estimators;
    estimators.reserve(params_.num_estimators);
    for (unsigned int round = 0; round < params_.num_estimators && labels.size() > 1; ++round) {
        std::vector<std::vector<double>> probabilities;
        probabilities.reserve(y.size());
        for (const auto& score : scores) {
            probabilities.push_back(softmax(score));
        }
        std::vector<std::unique_ptr<DecisionTreeRegressor>> trees;
        for (std::size_t k = 0; k < labels.size(); ++k) {
            std::vector<double> residuals(y.size());
            for (std::size_t i = 0; i < y.size(); ++i) {
                residuals[i] = (encoded[i] == k ? 1.0 : 0.0) - probabilities[i][k];
            }
            auto tree = std::make_unique<DecisionTreeRegressor>();
            tree->fit(X, residuals, static_cast<int>(params_.max_depth));
            for (std::size_t i = 0; i < y.size(); ++i) {
                scores[i][k] = update_score(scores[i][k], params_.learning_rate, tree->predict(X[i]));
            }
            trees.push_back(std::move(tree));
        }
        estimators.push_back(std::move(trees));
    }
    estimators_ = std::move(estimators);
    classes_ = std::move(labels);
    initial_scores_ = std::move(initial);
    num_features_ = features;
}

std::vector<double> BoostTreeClassifier::predict_proba(const std::vector<double>& sample) const {
    tree_ensemble_detail::validate_sample(sample, num_features_);
    auto scores = initial_scores_;
    for (const auto& round : estimators_) {
        for (std::size_t k = 0; k < scores.size(); ++k) {
            scores[k] = update_score(scores[k], params_.learning_rate, round[k]->predict(sample));
        }
    }
    return softmax(scores);
}

int BoostTreeClassifier::predict(const std::vector<double>& sample) const {
    const auto probabilities = predict_proba(sample);
    return classes_[std::max_element(probabilities.begin(), probabilities.end()) - probabilities.begin()];
}

const std::vector<int>& BoostTreeClassifier::classes() const {
    if (num_features_ == 0) {
        throw std::logic_error("Model must be fitted before requesting classes");
    }
    return classes_;
}
