#include <gtest/gtest.h>
#include "ml/dimensionality_reduction/knn.h"
#include <Eigen/Dense>

using namespace dimensionality_reduction;

class KNNTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create simple 2D dataset
        X = Eigen::MatrixXd(6, 2);
        X << 0.0, 0.0,
             1.0, 0.0,
             0.0, 1.0,
             10.0, 10.0,
             11.0, 10.0,
             10.0, 11.0;
    }
    
    Eigen::MatrixXd X;
};

TEST_F(KNNTest, FitKNN) {
    KNN knn(2);
    EXPECT_NO_THROW(knn.fit(X));
    EXPECT_TRUE(knn.is_fitted());
}

TEST_F(KNNTest, ThrowsIfNotFitted) {
    KNN knn(2);
    EXPECT_THROW(knn.kneighbors(), std::runtime_error);
}

TEST_F(KNNTest, FindNeighborsCorrect) {
    KNN knn(2);
    knn.fit(X);
    
    auto [indices, distances] = knn.kneighbors();
    
    EXPECT_EQ(indices.rows(), 6);
    EXPECT_EQ(indices.cols(), 2);
    
    // Check first point's neighbors (should be points 1 and 2)
    EXPECT_TRUE(indices(0, 0) == 1 || indices(0, 1) == 1);
    EXPECT_TRUE(indices(0, 0) == 2 || indices(0, 1) == 2);
}

TEST_F(KNNTest, QueryPoints) {
    KNN knn(2);
    knn.fit(X);
    
    Eigen::MatrixXd query(1, 2);
    query << 0.5, 0.5;
    
    auto [indices, distances] = knn.kneighbors(query);
    
    EXPECT_EQ(indices.rows(), 1);
    EXPECT_EQ(indices.cols(), 2);
}

TEST_F(KNNTest, DistanceMetrics) {
    // Test different metrics
    std::vector<std::string> metrics = {"euclidean", "manhattan", "cosine"};
    
    for (const auto& metric : metrics) {
        KNN knn(2, metric);
        EXPECT_NO_THROW(knn.fit(X));
        EXPECT_EQ(knn.get_metric(), metric);
    }
}

TEST_F(KNNTest, PairwiseDistances) {
    KNN knn(2);
    knn.fit(X);
    
    Eigen::MatrixXd Y(2, 2);
    Y << 0.0, 0.0,
         1.0, 1.0;
    
    Eigen::MatrixXd distances = knn.pairwise_distances(X, Y);
    
    EXPECT_EQ(distances.rows(), X.rows());
    EXPECT_EQ(distances.cols(), Y.rows());
    
    // Distance from (0,0) to (0,0) should be 0
    EXPECT_NEAR(distances(0, 0), 0.0, 1e-10);
}

TEST_F(KNNTest, InvalidK) {
    EXPECT_THROW(KNN knn(0), std::invalid_argument);
    EXPECT_THROW(KNN knn(-1), std::invalid_argument);
}

TEST_F(KNNTest, KTooLarge) {
    KNN knn(10);
    EXPECT_THROW(knn.fit(X), std::runtime_error);
}

TEST_F(KNNTest, EmptyData) {
    Eigen::MatrixXd empty(0, 0);
    KNN knn(2);
    EXPECT_THROW(knn.fit(empty), std::runtime_error);
}

TEST_F(KNNTest, CosineDistance) {
    Eigen::MatrixXd X_cos(3, 3);
    X_cos << 1.0, 0.0, 0.0,
             0.0, 1.0, 0.0,
             1.0, 1.0, 0.0;
    
    KNN knn(2, "cosine");
    knn.fit(X_cos);
    
    auto [indices, distances] = knn.kneighbors();
    
    // Point (1,1,0) should be closest to (1,0,0) and (0,1,0)
    EXPECT_EQ(indices.rows(), 3);
}
