#include "decision_tree.h"
#include <algorithm>
#include <numeric>
#include <set>
#include <limits>

// --- Helper functions ---

double calculate_gini_impurity(const std::vector<int>& y) {
    if (y.empty()) return 0.0;
    std::map<int, int> counts;
    for (int label : y) counts[label]++;
    double impurity = 1.0;
    double n = static_cast<double>(y.size());
    for (const auto& [label, count] : counts) {
        double p = count / n;
        impurity -= p * p;
    }
    return impurity;
}

double calculate_entropy(const std::vector<int>& y) {
    if (y.empty()) return 0.0;
    std::map<int, int> counts;
    for (int label : y) counts[label]++;
    double entropy = 0.0;
    double n = static_cast<double>(y.size());
    for (const auto& [label, count] : counts) {
        double p = count / n;
        if (p > 0) entropy -= p * std::log2(p);
    }
    return entropy;
}

// --- DecisionTree ---

DecisionTree::DecisionTree(SplitCriterion criterion)
    : root(nullptr), max_depth(0), criterion_(criterion) {
    if (criterion_ == SplitCriterion::ENTROPY)
        impurity_function_ = calculate_entropy;
    else
        impurity_function_ = calculate_gini_impurity;
}

DecisionTree::~DecisionTree() {
    delete_tree_recursive(root);
}

void DecisionTree::delete_tree_recursive(Node* node) {
    if (!node) return;
    for (auto& [val, child] : node->children)
        delete_tree_recursive(child);
    delete node;
}

void DecisionTree::fit(const std::vector<std::vector<int>>& X, const std::vector<int>& y, int max_depth) {
    delete_tree_recursive(root);
    this->max_depth = max_depth;
    root = build_tree_recursive(X, y, 0);
}

int DecisionTree::predict(const std::vector<int>& sample) const {
    Node* current = root;
    if (!current) return -1;
    while (!current->is_leaf) {
        int feature_val = sample[current->feature_index];
        auto it = current->children.find(feature_val);
        if (it == current->children.end()) {
            // Unknown feature value — return majority class of this subtree
            return current->class_label;
        }
        current = it->second;
    }
    return current->class_label;
}

int DecisionTree::find_best_split(const std::vector<std::vector<int>>& X, const std::vector<int>& y) {
    if (X.empty()) return -1;
    int num_features = static_cast<int>(X[0].size());
    double best_gain = -1.0;
    int best_feature = -1;
    double parent_impurity = impurity_function_(y);

    for (int f = 0; f < num_features; ++f) {
        // Group samples by feature value
        std::map<int, std::vector<int>> groups;
        for (size_t i = 0; i < X.size(); ++i)
            groups[X[i][f]].push_back(y[i]);

        // Calculate weighted impurity after split
        double weighted_impurity = 0.0;
        double n = static_cast<double>(y.size());
        for (const auto& [val, labels] : groups)
            weighted_impurity += (labels.size() / n) * impurity_function_(labels);

        double gain = parent_impurity - weighted_impurity;
        if (gain > best_gain) {
            best_gain = gain;
            best_feature = f;
        }
    }
    return best_feature;
}

Node* DecisionTree::build_tree_recursive(const std::vector<std::vector<int>>& X, const std::vector<int>& y, int current_depth) {
    Node* node = new Node();

    // Find majority class
    std::map<int, int> counts;
    for (int label : y) counts[label]++;
    int majority_class = -1;
    int max_count = 0;
    for (const auto& [label, count] : counts) {
        if (count > max_count) {
            max_count = count;
            majority_class = label;
        }
    }
    node->class_label = majority_class;

    // Check stopping conditions
    bool all_same = (counts.size() == 1);
    if (all_same || current_depth >= max_depth || X.empty()) {
        node->is_leaf = true;
        return node;
    }

    // Find best split
    int best_feature = find_best_split(X, y);
    if (best_feature < 0) {
        node->is_leaf = true;
        return node;
    }

    node->is_leaf = false;
    node->feature_index = best_feature;

    // Split data by feature value
    std::map<int, std::vector<size_t>> groups;
    for (size_t i = 0; i < X.size(); ++i)
        groups[X[i][best_feature]].push_back(i);

    for (const auto& [val, indices] : groups) {
        std::vector<std::vector<int>> X_sub;
        std::vector<int> y_sub;
        for (size_t idx : indices) {
            X_sub.push_back(X[idx]);
            y_sub.push_back(y[idx]);
        }
        node->children[val] = build_tree_recursive(X_sub, y_sub, current_depth + 1);
    }

    return node;
}
