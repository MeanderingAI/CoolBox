
#ifndef BOOST_TREE_H
#define BOOST_TREE_H

#include <vector>
#include "decision_tree_regressor.h"

/**
 * @struct BoostTreeParameters
 * @brief Defines the hyperparameters for the Boost Tree algorithm.
 */
struct BoostTreeParameters {
    unsigned int num_estimators = 100;
    double learning_rate = 0.1;
    unsigned int max_depth = 3;
};

/**
 * @class BoostTree
 * @brief An interface for a Gradient Boosted Decision Tree Regressor.
 *
 * This class uses an ensemble of weak learners (Decision Trees) to build a strong
 * predictive model using squared-error residuals. max_depth controls each tree;
 * zero produces constant learners. Invalid data/parameters throw
 * std::invalid_argument, and prediction before fit throws std::logic_error.
 * Failed fits preserve the previous model.
 */
class BoostTree {
private:
    // Model parameters
    BoostTreeParameters params_;

    // Immutable learners preserve independent refits when a fitted model is copied.
    std::vector<std::shared_ptr<const DecisionTreeRegressor>> estimators_;
    std::size_t num_features_ = 0;

    // The initial prediction, which serves as the starting point for the residuals
    double initial_prediction_ = 0.0;

public:
    /**
     * @brief Constructor that initializes the model with hyperparameters.
     * @param params The hyperparameters to configure the model.
     */
    BoostTree(const BoostTreeParameters& params);

    /**
     * @brief Destructor to clean up allocated memory.
     */
    ~BoostTree();

    /**
     * @brief Trains the Boost Tree model on the provided dataset.
     * * @param X The feature matrix for training (each inner vector is a sample).
     * @param y The target vector corresponding to the samples in X.
     */
    void fit(const std::vector<std::vector<double>>& X, const std::vector<double>& y);

    /**
     * @brief Predicts the output for a single sample.
     * * @param sample The feature vector for the sample to predict.
     * @return The predicted value.
     */
    double predict(const std::vector<double>& sample) const;

    /**
     * @brief Predicts the output for multiple samples (a batch).
     * * @param X The feature matrix for the samples to predict.
     * @return A vector of predicted values.
     */
    std::vector<double> predict(const std::vector<std::vector<double>>& X) const;

private:
    // Helper function for the prediction logic on a single sample.
    double predict_single(const std::vector<double>& sample) const;
};

/**
 * @brief Gradient boosting for binary or multiclass cross-entropy loss.
 *
 * Fits one regression tree per class per round to (one_hot(y) - probability).
 * Probabilities use softmax; their order matches classes(), sorted by label.
 * Single-class training is supported. Labels may be arbitrary integers.
 * Shares BoostTree's parameter, validation, and failed-fit semantics.
 */
class BoostTreeClassifier {
public:
    explicit BoostTreeClassifier(const BoostTreeParameters& params = BoostTreeParameters());
    BoostTreeClassifier(const BoostTreeClassifier&) = delete;
    BoostTreeClassifier& operator=(const BoostTreeClassifier&) = delete;
    void fit(const std::vector<std::vector<double>>& X, const std::vector<int>& y);
    int predict(const std::vector<double>& sample) const;
    std::vector<double> predict_proba(const std::vector<double>& sample) const;
    const std::vector<int>& classes() const;

private:
    BoostTreeParameters params_;
    std::size_t num_features_ = 0;
    std::vector<int> classes_;
    std::vector<double> initial_scores_;
    std::vector<std::vector<std::unique_ptr<DecisionTreeRegressor>>> estimators_;
};

#endif // BOOST_TREE_H