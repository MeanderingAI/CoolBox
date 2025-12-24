#include <random_forest.h>
#include <iostream>
#include <random>
#include <map>
#include <algorithm>

/**
 * @brief Constructor for the RandomForest classifier.
 * @param num_trees The number of decision trees in the forest.
 * @param max_depth The maximum depth for each individual tree.
 */
RandomForest::RandomForest(int num_trees, int max_depth)
    : num_trees_(num_trees), max_depth_(max_depth) {}

/**
 * @brief Destructor to clean up allocated memory.
 */
RandomForest::~RandomForest() {
    for (DecisionTree* tree : trees_) {
        delete tree;
    }
}

/**
 * @brief Private helper function to generate a bootstrap sample.
 */
void RandomForest::get_bootstrap_sample(
    const std::vector<std::vector<int>>& X_in,
    const std::vector<int>& y_in,
    std::vector<std::vector<int>>& X_out,
    std::vector<int>& y_out
) {
    X_out.clear();
    y_out.clear();
    int n_samples = X_in.size();
    
    // Use a fixed seed for reproducibility in tests, or a random device for real-world use.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, n_samples - 1);

    for (int i = 0; i < n_samples; ++i) {
        int index = distrib(gen);
        X_out.push_back(X_in[index]);
        y_out.push_back(y_in[index]);
    }
}

/**
 * @brief Trains the random forest using the provided dataset.
 * @param X The feature matrix.
 * @param y The target vector of class labels.
 */
void RandomForest::fit(const std::vector<std::vector<int>>& X, const std::vector<int>& y) {
    if (X.empty() || y.empty() || X.size() != y.size()) {
        std::cerr << "Error: Invalid training data." << std::endl;
        return;
    }
    
    // Create and train each tree in the forest.
    for (int i = 0; i < num_trees_; ++i) {
        DecisionTree* new_tree = new DecisionTree();
        
        std::vector<std::vector<int>> bootstrap_X;
        std::vector<int> bootstrap_y;
        get_bootstrap_sample(X, y, bootstrap_X, bootstrap_y);
        
        new_tree->fit(bootstrap_X, bootstrap_y, max_depth_);
        trees_.push_back(new_tree);
    }
}

/**
 * @brief Predicts the class label for a single sample.
 * @param sample The feature vector for the sample.
 * @return The predicted class label based on a majority vote.
 */
int RandomForest::predict(const std::vector<int>& sample) const {
    if (trees_.empty()) {
        return -1; // No trees to make a prediction.
    }

    std::map<int, int> vote_counts;
    
    for (const DecisionTree* tree : trees_) {
        int prediction = tree->predict(sample);
        vote_counts[prediction]++;
    }

    int majority_vote = -1;
    int max_count = -1;

    for (const auto& pair : vote_counts) {
        if (pair.second > max_count) {
            max_count = pair.second;
            majority_vote = pair.first;
        }
    }
    
    return majority_vote;
}
