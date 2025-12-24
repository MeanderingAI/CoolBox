#include <gtest/gtest.h>
#include <iostream>
#include <numeric>

#include "ml/decision_tree/decision_tree.h"

// Test fixtures for common setup (optional but good practice)
class DecisionTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // A simple dataset for testing
        // Features: X1, X2
        // Labels: y (0 or 1)
        // Expected split will be on X1.
        X_train = {
            {0, 0}, {0, 1},
            {1, 0}, {1, 1}
        };
        y_train = {0, 0, 1, 1};
    }

    std::vector<std::vector<int>> X_train;
    std::vector<int> y_train;
};

// Test the Gini impurity calculation
TEST_F(DecisionTreeTest, CalculateGiniImpurity) {
    // Test case 1: A perfect split (Gini = 0)
    std::vector<int> y_perfect = {0, 0, 0, 0};
    EXPECT_NEAR(calculate_gini_impurity(y_perfect), 0.0, 1e-9);

    // Test case 2: A 50/50 split (Gini = 0.5)
    std::vector<int> y_split = {0, 0, 1, 1};
    EXPECT_NEAR(calculate_gini_impurity(y_split), 0.5, 1e-9);

    // Test case 3: An uneven split
    std::vector<int> y_uneven = {0, 0, 0, 1};
    EXPECT_NEAR(calculate_gini_impurity(y_uneven), 1.0 - (0.75 * 0.75 + 0.25 * 0.25), 1e-9);

    // Test case 4: Empty vector
    std::vector<int> y_empty = {};
    EXPECT_NEAR(calculate_gini_impurity(y_empty), 0.0, 1e-9);
}

// Test the Entropy calculation
TEST_F(DecisionTreeTest, CalculateEntropy) {
    // Test case 1: A perfect split (Entropy = 0)
    std::vector<int> y_perfect = {0, 0, 0, 0};
    EXPECT_NEAR(calculate_entropy(y_perfect), 0.0, 1e-9);
    
    // Test case 2: A 50/50 split (Entropy = 1.0)
    std::vector<int> y_split = {0, 0, 1, 1};
    EXPECT_NEAR(calculate_entropy(y_split), 1.0, 1e-9);

    // Test case 3: An uneven split
    std::vector<int> y_uneven = {0, 0, 0, 1};
    EXPECT_NEAR(calculate_entropy(y_uneven), -(0.75 * std::log2(0.75) + 0.25 * std::log2(0.25)), 1e-9);

    // Test case 4: Empty vector
    std::vector<int> y_empty = {};
    EXPECT_NEAR(calculate_entropy(y_empty), 0.0, 1e-9);
}

// Test the tree-building and prediction logic with Gini
TEST_F(DecisionTreeTest, FitAndPredictWithGini) {
    DecisionTree tree(SplitCriterion::GINI);
    tree.fit(X_train, y_train, 2);

    // Test known samples
    EXPECT_EQ(tree.predict({0, 0}), 0);
    EXPECT_EQ(tree.predict({0, 1}), 0);
    EXPECT_EQ(tree.predict({1, 0}), 1);
    EXPECT_EQ(tree.predict({1, 1}), 1);
}

// Test the tree-building and prediction logic with Entropy
TEST_F(DecisionTreeTest, FitAndPredictWithEntropy) {
    DecisionTree tree(SplitCriterion::ENTROPY);
    tree.fit(X_train, y_train, 2);

    // Test known samples
    EXPECT_EQ(tree.predict({0, 0}), 0);
    EXPECT_EQ(tree.predict({0, 1}), 0);
    EXPECT_EQ(tree.predict({1, 0}), 1);
    EXPECT_EQ(tree.predict({1, 1}), 1);
}

// Test max depth constraint
TEST_F(DecisionTreeTest, MaxDepthConstraint) {
    // With max_depth = 0, the tree should be a single leaf returning the most common class.
    DecisionTree tree(SplitCriterion::GINI);
    std::vector<int> y_unbalanced = {0, 0, 0, 1, 1};
    tree.fit({{0, 0}, {0, 1}, {1, 0}, {1, 1}, {0, 0}}, y_unbalanced, 0);

    // The most common class is 0 (3 times).
    EXPECT_EQ(tree.predict({0, 0}), 0);
    EXPECT_EQ(tree.predict({1, 1}), 0);
}

// Test prediction with an unknown feature value
TEST_F(DecisionTreeTest, PredictWithUnseenFeatureValue) {
    DecisionTree tree(SplitCriterion::GINI);
    tree.fit(X_train, y_train, 2);

    // Sample with a feature value not seen during training.
    // The current implementation should return the class of the first child.
    EXPECT_EQ(tree.predict({2, 0}), 0);
}
