#include <gtest/gtest.h>
#include "ml/dimensionality_reduction/pca.h"
#include <Eigen/Dense>
#include <cmath>

using namespace dimensionality_reduction;

class PCATest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple dataset with correlation
        X = Eigen::MatrixXd(5, 3);
        X << 1.0, 2.0, 3.0,
             2.0, 3.0, 4.0,
             3.0, 4.0, 5.0,
             4.0, 5.0, 6.0,
             5.0, 6.0, 7.0;
    }
    
    Eigen::MatrixXd X;
};

TEST_F(PCATest, FitPCA) {
    PCA pca(2);
    EXPECT_NO_THROW(pca.fit(X));
    EXPECT_TRUE(pca.is_fitted());
}

TEST_F(PCATest, ThrowsIfNotFitted) {
    PCA pca;
    EXPECT_THROW(pca.transform(X), std::runtime_error);
    EXPECT_THROW(pca.get_components(), std::runtime_error);
}

TEST_F(PCATest, TransformDimensions) {
    PCA pca(2);
    pca.fit(X);
    
    Eigen::MatrixXd X_transformed = pca.transform(X);
    
    EXPECT_EQ(X_transformed.rows(), X.rows());
    EXPECT_EQ(X_transformed.cols(), 2);
}

TEST_F(PCATest, FitTransform) {
    PCA pca(2);
    Eigen::MatrixXd X_transformed = pca.fit_transform(X);
    
    EXPECT_EQ(X_transformed.rows(), X.rows());
    EXPECT_EQ(X_transformed.cols(), 2);
    EXPECT_TRUE(pca.is_fitted());
}

TEST_F(PCATest, InverseTransform) {
    PCA pca(2);
    Eigen::MatrixXd X_transformed = pca.fit_transform(X);
    Eigen::MatrixXd X_reconstructed = pca.inverse_transform(X_transformed);
    
    EXPECT_EQ(X_reconstructed.rows(), X.rows());
    EXPECT_EQ(X_reconstructed.cols(), X.cols());
}

TEST_F(PCATest, ReconstructionWithAllComponents) {
    PCA pca(0);  // Keep all components
    Eigen::MatrixXd X_transformed = pca.fit_transform(X);
    Eigen::MatrixXd X_reconstructed = pca.inverse_transform(X_transformed);
    
    // Should reconstruct almost perfectly
    double error = (X - X_reconstructed).norm() / X.norm();
    EXPECT_LT(error, 1e-8);
}

TEST_F(PCATest, ComponentsShape) {
    PCA pca(2);
    pca.fit(X);
    
    Eigen::MatrixXd components = pca.get_components();
    
    EXPECT_EQ(components.rows(), X.cols());
    EXPECT_EQ(components.cols(), 2);
}

TEST_F(PCATest, ExplainedVarianceRatioSumsToLessThanOne) {
    PCA pca(2);  // Only keep 2 components
    pca.fit(X);
    
    Eigen::VectorXd ratios = pca.get_explained_variance_ratio();
    double sum = ratios.sum();
    
    // Should sum to less than 1 if we're not keeping all components
    EXPECT_LE(sum, 1.0);
    EXPECT_EQ(ratios.size(), 2);
}

TEST_F(PCATest, ExplainedVarianceRatioSumsToOneWithAllComponents) {
    PCA pca(0);  // Keep all components
    pca.fit(X);
    
    Eigen::VectorXd ratios = pca.get_explained_variance_ratio();
    double sum = ratios.sum();
    
    EXPECT_NEAR(sum, 1.0, 1e-8);
}

TEST_F(PCATest, CenteringWorks) {
    PCA pca(0, true, false);
    pca.fit(X);
    
    Eigen::VectorXd mean = pca.get_mean();
    
    // Mean should be computed
    EXPECT_EQ(mean.size(), X.cols());
    
    // Check mean is correct
    Eigen::VectorXd expected_mean = X.colwise().mean();
    for (int i = 0; i < mean.size(); ++i) {
        EXPECT_NEAR(mean(i), expected_mean(i), 1e-10);
    }
}

TEST_F(PCATest, ScalingWorks) {
    PCA pca(0, true, true);
    pca.fit(X);
    
    Eigen::VectorXd scale = pca.get_scale();
    
    // Scale should be computed
    EXPECT_EQ(scale.size(), X.cols());
    
    // All scale values should be positive
    for (int i = 0; i < scale.size(); ++i) {
        EXPECT_GT(scale(i), 0.0);
    }
}

TEST_F(PCATest, NComponentsStored) {
    PCA pca(2);
    pca.fit(X);
    
    EXPECT_EQ(pca.get_n_components(), 2);
}

TEST_F(PCATest, EmptyMatrixThrows) {
    Eigen::MatrixXd empty(0, 0);
    PCA pca;
    
    EXPECT_THROW(pca.fit(empty), std::runtime_error);
}

TEST_F(PCATest, DimensionalityReduction) {
    // Create a dataset with clear principal directions
    Eigen::MatrixXd data(100, 3);
    for (int i = 0; i < 100; ++i) {
        data(i, 0) = i;
        data(i, 1) = i + 0.1 * (rand() % 100);
        data(i, 2) = 0.01 * (rand() % 100);  // Noise dimension
    }
    
    PCA pca(2);
    pca.fit(data);
    
    Eigen::VectorXd ratios = pca.get_explained_variance_ratio();
    
    // First two components should explain most of the variance
    double explained_by_first_two = ratios(0) + ratios(1);
    EXPECT_GT(explained_by_first_two, 0.9);
}

TEST_F(PCATest, TransformNewData) {
    PCA pca(2);
    pca.fit(X);
    
    // Create new data
    Eigen::MatrixXd X_new(2, 3);
    X_new << 6.0, 7.0, 8.0,
             7.0, 8.0, 9.0;
    
    EXPECT_NO_THROW(pca.transform(X_new));
    
    Eigen::MatrixXd X_new_transformed = pca.transform(X_new);
    EXPECT_EQ(X_new_transformed.rows(), 2);
    EXPECT_EQ(X_new_transformed.cols(), 2);
}
