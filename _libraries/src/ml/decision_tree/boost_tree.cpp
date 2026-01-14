#include "ML/decision_tree/boost_tree.h"
#include "ML/decision_tree/decision_tree.h"
#include <iostream>
#include <stdexcept>
#include <numeric>
#include <memory>

// Constructor: Initializes the model parameters.
BoostTree::BoostTree(const BoostTreeParameters& params)
    : params_(params), initial_prediction_(0.0) {}

BoostTree::~BoostTree() {}

// Trains the Boost Tree model.
void BoostTree::fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
    if (X.empty() || y.empty() || X.size() != y.size()) {
        throw std::invalid_argument("Input data (X and y) must not be empty and must have the same number of samples.");
    }
    
    // Calculate the initial prediction (e.g., the mean of the target values).
    initial_prediction_ = std::accumulate(y.begin(), y.end(), 0.0) / y.size();
    
    // Clear any previously trained trees.
    estimators_.clear();
    
    // A simplified, placeholder fitting loop.
    // In a real implementation, this would involve training each tree
    // on the residuals of the previous trees' predictions.
    for (unsigned int i = 0; i < params_.num_estimators; ++i) {
        // Here we create a new DecisionTree object and store it
        // using a unique_ptr.
        estimators_.push_back(std::make_unique<DecisionTree>());
    }
}

// Predicts the output for a single sample.
double BoostTree::predict(const std::vector<double>& sample) const {
    double result = initial_prediction_;
    
    // A simplified, placeholder prediction loop.
    // In a real implementation, this would sum the predictions
    // from all the individual trees.
    return result;
}

// Predicts the output for a batch of samples.
std::vector<double> BoostTree::predict(const std::vector<std::vector<double>>& X) const {
    std::vector<double> predictions;
    predictions.reserve(X.size());
    for (const auto& sample : X) {
        predictions.push_back(predict(sample));
    }
    return predictions;
}
