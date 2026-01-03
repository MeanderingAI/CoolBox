
#ifndef DECISION_TREE_H
#define DECISION_TREE_H

#include "lib_metadata.h"
// Metadata only defined in decision_tree.cpp to avoid duplicate symbols

#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <functional>

// Forward declaration for the RuleSet class
class RuleSet;

// Represents a single node in the decision tree.
struct Node {
    // A flag to check if the node is a leaf node.
    bool is_leaf;
    // The class label if it is a leaf node.
    int class_label;
    // The index of the feature to split on at this node.
    int feature_index;
    // A map from a feature value to a child node.
    // The key is the value of the feature, and the value is a pointer to the next node.
    std::map<int, Node*> children;
};

// Enum to select the splitting criterion.
enum class SplitCriterion {
    GINI,
    ENTROPY
};

// Helper functions (declared in the global scope).
double calculate_gini_impurity(const std::vector<int>& y);
double calculate_entropy(const std::vector<int>& y);

// Represents a Decision Tree classifier.
class DecisionTree {
public:
    // Constructor.
    DecisionTree(SplitCriterion criterion = SplitCriterion::GINI);
    
    // Destructor to clean up memory.
    ~DecisionTree();

    /**
     * @brief Trains the decision tree using the provided dataset.
     * @param X The feature matrix, where each row is a sample and each column is a feature.
     * @param y The target vector of class labels.
     * @param max_depth The maximum depth of the tree to prevent overfitting.
     */
    void fit(const std::vector<std::vector<int>>& X, const std::vector<int>& y, int max_depth);

    /**
     * @brief Predicts the class label for a single sample.
     * @param sample The feature vector for the sample to classify.
     * @return The predicted class label.
     */
    int predict(const std::vector<int>& sample) const;

private:
    Node* root;
    int max_depth;
    SplitCriterion criterion_;
    std::function<double(const std::vector<int>&)> impurity_function_;

    // Helper function to recursively build the tree.
    Node* build_tree_recursive(const std::vector<std::vector<int>>& X, const std::vector<int>& y, int current_depth);
    
    // Helper function to find the best feature to split on.
    int find_best_split(const std::vector<std::vector<int>>& X, const std::vector<int>& y);

    // Helper function to delete the tree.
    void delete_tree_recursive(Node* node);

    friend class RuleSet; // Allow RuleSet to access private members.

    // The functions are declared as friends to give them access to private members.
    friend double calculate_gini_impurity(const std::vector<int>& y);
    friend double calculate_entropy(const std::vector<int>& y);
};

#endif // DECISION_TREE_H