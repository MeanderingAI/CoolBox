
#ifndef BOOST_TREE_H
#define BOOST_TREE_H

#include "lib_metadata.h"
// Metadata only defined in decision_tree.h to avoid duplicate symbols

#include <vector>
#include "mytrix_eigen_compat.hpp"

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
 * predictive model. It can be extended for regression and classification tasks.
 */
class BoostTree {
private:
    struct WeakLearner {
        size_t feature_index = 0;
        double threshold = 0.0;
        double left_value = 0.0;
        double right_value = 0.0;
        bool has_split = false;
    };

    // Model parameters
    BoostTreeParameters params_;

    // The collection of trained weak learners.
    std::vector<WeakLearner> estimators_;

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

#endif // BOOST_TREE_H