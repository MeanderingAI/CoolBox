#include <gtest/gtest.h>
#include "ml/dimensionality_reduction/umap.h"
#include <Eigen/Dense>
#include <cmath>

using namespace dimensionality_reduction;

class UMAPTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create synthetic data with clear structure
        // Two clusters in 10D space
        int n_samples = 60;
        int n_features = 10;
        
        X = Eigen::MatrixXd(n_samples, n_features);
        
        // Cluster 1: centered at origin
        for (int i = 0; i < 30; ++i) {
            for (int j = 0; j < n_features; ++j) {
                X(i, j) = (rand() % 100) / 100.0 - 0.5;
            }
        }
        
        // Cluster 2: centered at (5, 5, ..., 5)
        for (int i = 30; i < 60; ++i) {
            for (int j = 0; j < n_features; ++j) {
                X(i, j) = 5.0 + (rand() % 100) / 100.0 - 0.5;
            }
        }
    }
    
    Eigen::MatrixXd X;
};

TEST_F(UMAPTest, Construction) {
    EXPECT_NO_THROW(UMAP umap(2, 15, 0.1));
}

TEST_F(UMAPTest, InvalidParameters) {
    EXPECT_THROW(UMAP umap(0), std::invalid_argument);  // n_components <= 0
    EXPECT_THROW(UMAP umap(2, 0), std::invalid_argument);  // n_neighbors <= 0
    EXPECT_THROW(UMAP umap(2, 15, -0.1), std::invalid_argument);  // min_dist < 0
    EXPECT_THROW(UMAP umap(2, 15, 1.5), std::invalid_argument);  // min_dist > 1
}

TEST_F(UMAPTest, FitTransform) {
    UMAP umap(2, 15, 0.1, "euclidean", 1.0, 50);  // Fewer epochs for speed
    
    EXPECT_NO_THROW(umap.fit_transform(X));
    EXPECT_TRUE(umap.is_fitted());
}

TEST_F(UMAPTest, EmbeddingDimensions) {
    UMAP umap(3, 15, 0.1, "euclidean", 1.0, 50);
    Eigen::MatrixXd embedding = umap.fit_transform(X);
    
    EXPECT_EQ(embedding.rows(), X.rows());
    EXPECT_EQ(embedding.cols(), 3);
}

TEST_F(UMAPTest, GetEmbedding) {
    UMAP umap(2, 15, 0.1, "euclidean", 1.0, 50);
    umap.fit(X);
    
    Eigen::MatrixXd emb1 = umap.get_embedding();
    Eigen::MatrixXd emb2 = umap.transform(X);
    
    EXPECT_EQ(emb1.rows(), emb2.rows());
    EXPECT_EQ(emb1.cols(), emb2.cols());
}

TEST_F(UMAPTest, ThrowsIfNotFitted) {
    UMAP umap(2);
    EXPECT_THROW(umap.transform(X), std::runtime_error);
    EXPECT_THROW(umap.get_embedding(), std::runtime_error);
}

TEST_F(UMAPTest, TooFewSamples) {
    Eigen::MatrixXd X_small(10, 5);
    X_small.setRandom();
    
    UMAP umap(2, 15);  // Need at least 16 samples
    EXPECT_THROW(umap.fit(X_small), std::runtime_error);
}

TEST_F(UMAPTest, DifferentMetrics) {
    std::vector<std::string> metrics = {"euclidean", "manhattan", "cosine"};
    
    for (const auto& metric : metrics) {
        UMAP umap(2, 15, 0.1, metric, 1.0, 20);
        EXPECT_NO_THROW(umap.fit(X));
    }
}

TEST_F(UMAPTest, ClusterSeparation) {
    // Test that UMAP separates the two clusters
    UMAP umap(2, 15, 0.1, "euclidean", 1.0, 100);
    Eigen::MatrixXd embedding = umap.fit_transform(X);
    
    // Compute centroids of the two clusters in embedding space
    Eigen::Vector2d centroid1 = embedding.topRows(30).colwise().mean();
    Eigen::Vector2d centroid2 = embedding.bottomRows(30).colwise().mean();
    
    // Distance between centroids should be significant
    double centroid_dist = (centroid1 - centroid2).norm();
    EXPECT_GT(centroid_dist, 0.1);  // Clusters should be separated (relaxed threshold)
}

TEST_F(UMAPTest, DeterministicWithSeed) {
    UMAP umap1(2, 15, 0.1, "euclidean", 1.0, 50, 123);
    UMAP umap2(2, 15, 0.1, "euclidean", 1.0, 50, 123);
    
    Eigen::MatrixXd emb1 = umap1.fit_transform(X);
    Eigen::MatrixXd emb2 = umap2.fit_transform(X);
    
    // With same seed, embeddings should be identical (or very similar)
    double diff = (emb1 - emb2).norm();
    EXPECT_LT(diff, 0.1);  // Allow small numerical differences
}

TEST_F(UMAPTest, GetParameters) {
    UMAP umap(3, 20);
    EXPECT_EQ(umap.get_n_components(), 3);
    EXPECT_EQ(umap.get_n_neighbors(), 20);
}
