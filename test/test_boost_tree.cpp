#include <gtest/gtest.h>
#include "ml/decision_tree/boost_tree.h"
#include <iostream>
#include <numeric>

// Helper to check if two doubles are approximately equal.
bool nearly_equal(double a, double b, double epsilon = 1e-6) {
    return std::abs(a - b) < epsilon;
}

class BoostTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // A simple dataset for testing.
        X_train = {{1}, {2}, {3}, {4}, {5}};
        y_train = {7.0, 9.0, 11.0, 13.0, 15.0};

        // Separate test data.
        X_test = {{6}, {7}, {8}};
        y_test = {17.0, 19.0, 21.0};
    }

    std::vector<std::vector<double>> X_train;
    std::vector<double> y_train;
    std::vector<std::vector<double>> X_test;
    std::vector<double> y_test;
};

// This test verifies the behavior of the previous, placeholder implementation,
// where the model always predicts the mean of the training data.
TEST_F(BoostTreeTest, PredictsMeanOfTrainingData) {
    BoostTreeParameters params;
    params.num_estimators = 10;
    
    BoostTree model(params);
    model.fit(X_train, y_train);

    // Calculate the expected mean of the training data.
    double expected_mean = std::accumulate(y_train.begin(), y_train.end(), 0.0) / y_train.size();

    // The prediction for a known sample should be the mean.
    // Use std::vector<double>{5.0} to be explicit with the type.
    double prediction_known = model.predict(std::vector<double>{5.0});
    EXPECT_TRUE(nearly_equal(prediction_known, expected_mean));

    // The prediction for an unseen sample should also be the mean.
    // Use std::vector<double>{100.0} to be explicit with the type.
    double prediction_unseen = model.predict(std::vector<double>{100.0});
    EXPECT_TRUE(nearly_equal(prediction_unseen, expected_mean));
}

// This test verifies that the `predict` method for multiple samples
// also returns the mean of the training data for each sample.
TEST_F(BoostTreeTest, PredictMultipleSamplesReturnsMean) {
    BoostTreeParameters params;
    params.num_estimators = 10;
    
    BoostTree model(params);
    model.fit(X_train, y_train);

    // Calculate the expected mean of the training data.
    double expected_mean = std::accumulate(y_train.begin(), y_train.end(), 0.0) / y_train.size();

    // Get predictions for the test set.
    std::vector<double> predictions = model.predict(X_test);

    ASSERT_EQ(predictions.size(), y_test.size());

    // Each prediction should be the same as the expected mean.
    for (size_t i = 0; i < predictions.size(); ++i) {
        EXPECT_TRUE(nearly_equal(predictions[i], expected_mean));
    }
}