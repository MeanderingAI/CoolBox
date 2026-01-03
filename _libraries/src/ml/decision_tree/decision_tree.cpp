#include "lib_metadata.h"

// Only one definition of metadata per library
LIBRARY_METADATA(
    decision_tree,
    "DecisionTree",
    "1.0",
    "Decision Tree for classification and regression tasks.",
    "ToolBox Authors"
)
#include "ml/decision_tree/decision_tree.h"
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <algorithm>

// Friend function to compute the Gini impurity.
double calculate_gini_impurity(const std::vector<int>& y) {
    if (y.empty()) {
        return 0.0;
    }
    std::map<int, int> class_counts;
    for (int label : y) {
        class_counts[label]++;
    }

    double gini = 1.0;
    for (const auto& pair : class_counts) {
        double prob = static_cast<double>(pair.second) / y.size();
        gini -= prob * prob;
    }
    return gini;
}

// Friend function to compute the Entropy.
double calculate_entropy(const std::vector<int>& y) {
    if (y.empty()) {
        return 0.0;
    }
    std::map<int, int> class_counts;
    for (int label : y) {
        class_counts[label]++;
    }

    double entropy = 0.0;
    for (const auto& pair : class_counts) {
        double prob = static_cast<double>(pair.second) / y.size();
        if (prob > 0) {
            entropy -= prob * std::log2(prob);
        }
    }
    return entropy;
}

// Constructor
DecisionTree::DecisionTree(SplitCriterion criterion)
    : root(nullptr), criterion_(criterion) {
    if (criterion_ == SplitCriterion::GINI) {
        impurity_function_ = calculate_gini_impurity;
    } else {
        impurity_function_ = calculate_entropy;
    }
}

// Destructor
DecisionTree::~DecisionTree() {
    delete_tree_recursive(root);
}

// Helper function to delete the tree recursively.
void DecisionTree::delete_tree_recursive(Node* node) {
    if (node == nullptr) {
        return;
    }
    for (auto const& [key, val] : node->children) {
        delete_tree_recursive(val);
    }
    delete node;
}

// Helper function to recursively build the tree.
Node* DecisionTree::build_tree_recursive(const std::vector<std::vector<int>>& X, const std::vector<int>& y, int current_depth) {
    Node* node = new Node();
    node->is_leaf = false;
    
    // Check for base cases
    // Base Case 1: All samples have the same class label.
    if (std::all_of(y.begin() + 1, y.end(), [&](int label){ return label == y[0]; })) {
        node->is_leaf = true;
        node->class_label = y[0];
        return node;
    }

    // Base Case 2: Max depth reached.
    if (current_depth >= max_depth) {
        node->is_leaf = true;
        // Assign the most common class as the label.
        std::map<int, int> class_counts;
        for (int label : y) {
            class_counts[label]++;
        }
        int most_common_label = 0;
        int max_count = 0;
        for (const auto& pair : class_counts) {
            if (pair.second > max_count) {
                max_count = pair.second;
                most_common_label = pair.first;
            }
        }
        node->class_label = most_common_label;
        return node;
    }

    // Find the best feature to split on.
    int best_split_feature = find_best_split(X, y);

    // Base Case 3: No good split found.
    if (best_split_feature == -1) {
        node->is_leaf = true;
        std::map<int, int> class_counts;
        for (int label : y) {
            class_counts[label]++;
        }
        int most_common_label = 0;
        int max_count = 0;
        for (const auto& pair : class_counts) {
            if (pair.second > max_count) {
                max_count = pair.second;
                most_common_label = pair.first;
            }
        }
        node->class_label = most_common_label;
        return node;
    }

    node->feature_index = best_split_feature;

    // Split the dataset based on the best feature.
    std::map<int, std::vector<std::vector<int>>> child_X;
    std::map<int, std::vector<int>> child_y;
    for (size_t i = 0; i < X.size(); ++i) {
        int feature_value = X[i][best_split_feature];
        child_X[feature_value].push_back(X[i]);
        child_y[feature_value].push_back(y[i]);
    }

    // Recursively build child nodes.
    for (const auto& pair : child_X) {
        node->children[pair.first] = build_tree_recursive(pair.second, child_y.at(pair.first), current_depth + 1);
    }

    return node;
}

// Helper function to find the best feature to split on.
int DecisionTree::find_best_split(const std::vector<std::vector<int>>& X, const std::vector<int>& y) {
    double current_impurity = impurity_function_(y);
    double best_information_gain = -1.0;
    int best_feature_index = -1;

    int num_features = X.empty() ? 0 : X[0].size();
    for (int feature_index = 0; feature_index < num_features; ++feature_index) {
        std::map<int, std::vector<int>> child_ys;
        std::set<int> unique_values;
        for (size_t i = 0; i < X.size(); ++i) {
            int feature_value = X[i][feature_index];
            child_ys[feature_value].push_back(y[i]);
            unique_values.insert(feature_value);
        }

        double weighted_child_impurity = 0.0;
        for (const auto& pair : child_ys) {
            double prob = static_cast<double>(pair.second.size()) / y.size();
            weighted_child_impurity += prob * impurity_function_(pair.second);
        }
        
        double information_gain = current_impurity - weighted_child_impurity;

        if (information_gain > best_information_gain) {
            best_information_gain = information_gain;
            best_feature_index = feature_index;
        }
    }
    return best_feature_index;
}

// Trains the decision tree.
void DecisionTree::fit(const std::vector<std::vector<int>>& X, const std::vector<int>& y, int max_depth) {
    if (root) {
        delete_tree_recursive(root);
    }
    this->max_depth = max_depth;
    root = build_tree_recursive(X, y, 0);
}

// Predicts the class label for a single sample.
int DecisionTree::predict(const std::vector<int>& sample) const {
    if (root == nullptr) {
        return -1; // Or throw an exception
    }

    Node* current_node = root;
    while (!current_node->is_leaf) {
        int feature_value = sample[current_node->feature_index];
        if (current_node->children.count(feature_value)) {
            current_node = current_node->children.at(feature_value);
        } else {
            // Handle unseen feature values by falling back to the most common class in the current node.
            // This is a simple form of handling unknown data.
            std::map<int, int> class_counts;
            // A more robust implementation would require more data or a different strategy.
            // For this basic implementation, we can return the class of the parent node.
            return current_node->children.begin()->second->class_label;
        }
    }
    return current_node->class_label;
}
