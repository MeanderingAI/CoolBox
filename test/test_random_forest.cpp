#include <gtest/gtest.h>
#include "ml/decision_tree/random_forest.h"
#include "ml/decision_tree/decision_tree.h"
#include <vector>
#include <numeric>
#include <random>
#include <algorithm>
// A test fixture to set up a simple dataset for testing
class RandomForestTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple binary classification dataset
        // Feature 0: weather (0=sunny, 1=rainy)
        // Feature 1: is_hot (0=no, 1=yes)
        // Label: go_out (0=no, 1=yes)
        X = {
            {0, 0}, {0, 0}, {0, 0}, // sunny, cold -> go_out
            {0, 1}, {0, 1}, // sunny, hot -> go_out
            {1, 0}, {1, 0}, // rainy, cold -> stay_in
            {1, 1}, {1, 1}, {1, 1} // rainy, hot -> stay_in
        };
        y = {1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
    }

    std::vector<std::vector<int>> X;
    std::vector<int> y;
};

// Test to ensure the random forest can be trained without crashing
TEST_F(RandomForestTest, FitMethodWorks) {
    RandomForest forest(10, 3); // 10 trees, max depth 3
    ASSERT_NO_THROW(forest.fit(X, y));
}

// Test that the random forest can make predictions
TEST_F(RandomForestTest, PredictMethodWorks) {
    RandomForest forest(10, 3);
    forest.fit(X, y);

    // Test a sample that should be classified as "go_out" (class 1)
    std::vector<int> sunny_day_sample = {0, 1}; // sunny, hot
    ASSERT_EQ(forest.predict(sunny_day_sample), 1);

    // Test a sample that should be classified as "stay_in" (class 0)
    std::vector<int> rainy_day_sample = {1, 0}; // rainy, cold
    ASSERT_EQ(forest.predict(rainy_day_sample), 0);
}

// Test that an empty forest returns a default value
TEST(RandomForestEdgeTest, PredictEmptyForest) {
    RandomForest forest(0, 3); // 0 trees
    std::vector<int> sample = {0, 0};
    ASSERT_EQ(forest.predict(sample), -1);
}
