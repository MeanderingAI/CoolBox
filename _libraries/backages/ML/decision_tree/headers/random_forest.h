
#ifndef RANDOM_FOREST_H
#define RANDOM_FOREST_H

#include "lib_metadata.h"
// Metadata only defined in decision_tree.h to avoid duplicate symbols

#include <vector>
#include <memory>
#include "ml/decision_tree/decision_tree.h"

class RandomForest {
public:
    /**
     * @brief Constructor for the RandomForest classifier.
     * @param num_trees The number of decision trees in the forest.
     * @param max_depth The maximum depth for each individual tree.
     */
    RandomForest(int num_trees, int max_depth);

    /**
     * @brief Destructor to clean up allocated memory.
     */
    ~RandomForest();

    /**
     * @brief Trains the random forest using the provided dataset.
     * @param X The feature matrix.
     * @param y The target vector of class labels.
     */
    void fit(const std::vector<std::vector<int>>& X, const std::vector<int>& y);

    /**
     * @brief Predicts the class label for a single sample.
     * @param sample The feature vector for the sample.
     * @return The predicted class label based on a majority vote.
     */
    int predict(const std::vector<int>& sample) const;

private:
    int num_trees_;
    int max_depth_;
    std::vector<DecisionTree*> trees_;

    // Private helper function to generate a bootstrap sample.
    void get_bootstrap_sample(
        const std::vector<std::vector<int>>& X_in,
        const std::vector<int>& y_in,
        std::vector<std::vector<int>>& X_out,
        std::vector<int>& y_out
    );
};

#endif // RANDOM_FOREST_H
