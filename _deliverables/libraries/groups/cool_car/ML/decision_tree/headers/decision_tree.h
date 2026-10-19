

/**
 * @page decision_tree_main DecisionTree Library
 *
 * @section usage_examples_decision_tree Usage Examples
 *
 * @subsection cpp_example_decision_tree C++ Example
 * @code{.cpp}
 * #include "ml/decision_tree/decision_tree.h"
 * DecisionTree tree(SplitCriterion::GINI);
 * tree.fit(X, y, 5); // X: std::vector<std::vector<int>>, y: std::vector<int>
 * int label = tree.predict(sample);
 * @endcode
 *
 * Numeric regression uses a separate API, leaving the categorical classifier
 * and its integer labels unchanged. Include decision_tree.h or
 * decision_tree_regressor.h.
 * @code{.cpp}
 * DecisionTreeRegressor regressor;
 * regressor.fit({{0.0}, {1.0}, {2.0}, {3.0}}, {0.5, 0.5, 4.5, 4.5}, 2);
 * double value = regressor.predict({2.5}); // 4.5
 * @endcode
 * Python exposes ml_core.decision_tree.DecisionTreeRegressor with the same
 * fit(X, y, max_depth) and predict(sample) methods. JavaScript exposes
 * Module.DecisionTreeRegressor using VectorVectorDouble_DT and VectorDouble_DT.
 * Regression uses numeric threshold splits minimizing squared error and mean
 * leaf predictions. max_depth=0 predicts the overall mean. Invalid training
 * data or samples throw exceptions; prediction before fitting is an error.
 *
 * @subsection tree_ensembles Tree Ensembles
 * Include random_forest.h for RandomForest (integer/categorical classification)
 * and RandomForestRegressor (numeric regression). The latter averages
 * bootstrapped trees and accepts an optional reproducible seed.
 * Include boost_tree.h for BoostTree (squared-error regression) and
 * BoostTreeClassifier (binary/multiclass cross-entropy classification).
 * Both boosting variants fit DecisionTreeRegressor learners, honoring
 * BoostTreeParameters::max_depth rather than restricting learners to stumps.
 * BoostTreeClassifier::predict_proba returns probabilities in the sorted
 * integer-label order returned by classes(). Ties select the first label.
 * @code{.cpp}
 * BoostTreeParameters params;
 * params.max_depth = 2;
 * BoostTreeClassifier classifier(params);
 * classifier.fit({{0.0}, {1.0}, {2.0}}, {-1, 4, 9});
 * auto probabilities = classifier.predict_proba({1.0});
 * RandomForestRegressor forest(50, 3, 123);
 * forest.fit({{0.0}, {1.0}}, {1.5, 4.5});
 * double prediction = forest.predict({0.8});
 * @endcode
 * Python exposes these numeric ensemble classes under ml_core.decision_tree;
 * JavaScript exposes them on Module. BoostTree's Python batch prediction is
 * predict_batch(X). As with the existing forest, regression forests use all
 * features per split, not random feature subsampling.
 *
 * @subsection python_example_decision_tree Python Example
 * @code{.python}
 * from ml_core.decision_tree import DecisionTree, SplitCriterion
 * tree = DecisionTree(SplitCriterion.GINI)
 * tree.fit(X, y, 5)
 * label = tree.predict(sample)
 * @endcode
 *
 * @subsection js_example_decision_tree JavaScript Example (WASM/Emscripten)
 * @code{.js}
 * // Async usage (MODULARIZE=1, default):
 * createDecisionTreeModule().then(Module => {
 *     const DecisionTree = Module.DecisionTree;
 *     const tree = new DecisionTree(Module.SplitCriterion.GINI);
 *     tree.fit(X, y, 5);
 *     const label = tree.predict(sample);
 * });
 * @endcode
 *
 * @subsection js_example_sync_decision_tree JavaScript Example (Synchronous, MODULARIZE=0)
 * @code{.js}
 * // If decision_tree.js is loaded and exposes 'Module' globally:
 * const DecisionTree = Module.DecisionTree;
 * const tree = new DecisionTree(Module.SplitCriterion.GINI);
 * tree.fit(X, y, 5);
 * const label = tree.predict(sample);
 * // Note: If built with MODULARIZE=1 (default), you must use createDecisionTreeModule().then(...)
 * // If built with MODULARIZE=0, you can use the Module object directly after script load.
 * @endcode
 */
#ifndef DECISION_TREE_H
#define DECISION_TREE_H

#include "lib_metadata.h"
#include "decision_tree_regressor.h"
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