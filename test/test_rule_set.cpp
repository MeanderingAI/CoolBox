#include <gtest/gtest.h>
#include "ml/decision_tree/decision_tree.h"
#include "ml/decision_tree/rule_set.h"
#include <vector>
#include <map>
#include <string>

// A test fixture to set up a common decision tree for the tests
class RuleSetTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple dataset
        // Features: [0: weather (0=sunny, 1=rainy), 1: hot (0=cold, 1=hot)]
        // Labels: [class (0=stay_in, 1=go_out)]
        std::vector<std::vector<int>> X = {
            {0, 0}, // sunny, cold -> go_out
            {0, 1}, // sunny, hot  -> go_out
            {1, 0}, // rainy, cold -> stay_in
            {1, 1}  // rainy, hot  -> stay_in
        };
        std::vector<int> y = {1, 1, 0, 0};
        
        // Train a decision tree
        tree.fit(X, y, 2);
    }

    DecisionTree tree;
};

// Test to ensure the rules are correctly generated from the tree
TEST_F(RuleSetTest, RulesAreGeneratedCorrectly) {
    // Convert the trained tree to a rule set
    RuleSet ruleset(tree);

    // Get the generated rules as strings
    const auto& rules = ruleset.get_rules();

    // The tree should split on feature 0 (weather), leading to two rules
    ASSERT_EQ(rules.size(), 2);

    // Check for the expected rule strings
    std::vector<std::string> expected_rules = {
        "IF feature[0] == 0 THEN class is 1", // Weather is sunny -> go_out
        "IF feature[0] == 1 THEN class is 0"  // Weather is rainy -> stay_in
    };

    // Use a set to compare without worrying about order
    std::set<std::string> expected_set(expected_rules.begin(), expected_rules.end());
    std::set<std::string> actual_set(rules.begin(), rules.end());

    ASSERT_EQ(expected_set, actual_set);
}

// Test to ensure predictions are correct using the generated rules
TEST_F(RuleSetTest, PredictionsAreCorrect) {
    // Convert the trained tree to a rule set
    RuleSet ruleset(tree);

    // Test a sample that should match a rule
    std::vector<int> sunny_sample = {0, 0}; // sunny, cold
    ASSERT_EQ(ruleset.predict(sunny_sample), 1);

    // Test another sample that should match a rule
    std::vector<int> rainy_sample = {1, 1}; // rainy, hot
    ASSERT_EQ(ruleset.predict(rainy_sample), 0);

    // Test a sample with a non-relevant feature that should still be predicted correctly
    std::vector<int> another_sunny_sample = {0, 1}; // sunny, hot
    ASSERT_EQ(ruleset.predict(another_sunny_sample), 1);
}

// Test to ensure prediction returns -1 for an un-matched sample
TEST_F(RuleSetTest, NoRuleMatch) {
    // Convert the trained tree to a rule set
    RuleSet ruleset(tree);

    // A sample with a feature that is not in the training data
    std::vector<int> unknown_sample = {2, 0}; 
    ASSERT_EQ(ruleset.predict(unknown_sample), -1);
}