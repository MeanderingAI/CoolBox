#include <gtest/gtest.h>
#include "ml/dimensionality_reduction/svd.h"
#include <Eigen/Dense>
#include <cmath>

using namespace dimensionality_reduction;

class SVDTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple 4x3 matrix for testing
        X = Eigen::MatrixXd(4, 3);
        X << 1.0, 2.0, 3.0,
             4.0, 5.0, 6.0,
             7.0, 8.0, 9.0,
             10.0, 11.0, 12.0;
    }
    
    Eigen::MatrixXd X;
};

TEST_F(SVDTest, ComputeSVD) {
    SVD svd;
    EXPECT_NO_THROW(svd.compute(X));
    EXPECT_TRUE(svd.is_computed());
}

TEST_F(SVDTest, ThrowsIfNotComputed) {
    SVD svd;
    EXPECT_THROW(svd.get_U(), std::runtime_error);
    EXPECT_THROW(svd.get_V(), std::runtime_error);
    EXPECT_THROW(svd.get_singular_values(), std::runtime_error);
}

TEST_F(SVDTest, DimensionsCorrect) {
    SVD svd;
    svd.compute(X);
    
    Eigen::MatrixXd U = svd.get_U();
    Eigen::VectorXd S = svd.get_singular_values();
    Eigen::MatrixXd V = svd.get_V();
    
    // For thin SVD: U is m x k, S is k, V is n x k where k = min(m,n)
    EXPECT_EQ(U.rows(), 4);
    EXPECT_EQ(U.cols(), 3);
    EXPECT_EQ(S.size(), 3);
    EXPECT_EQ(V.rows(), 3);
    EXPECT_EQ(V.cols(), 3);
}

TEST_F(SVDTest, Reconstruction) {
    SVD svd;
    svd.compute(X);
    
    Eigen::MatrixXd X_reconstructed = svd.reconstruct();
    
    // Check reconstruction is close to original
    double error = (X - X_reconstructed).norm() / X.norm();
    EXPECT_LT(error, 1e-10);
}

TEST_F(SVDTest, PartialReconstruction) {
    SVD svd;
    svd.compute(X);
    
    // Reconstruct with only 2 components
    Eigen::MatrixXd X_approx = svd.reconstruct(2);
    
    EXPECT_EQ(X_approx.rows(), X.rows());
    EXPECT_EQ(X_approx.cols(), X.cols());
    
    // Approximation should have some error
    double error = (X - X_approx).norm() / X.norm();
    EXPECT_GT(error, 0.0);
}

TEST_F(SVDTest, SingularValuesDecreasing) {
    SVD svd;
    svd.compute(X);
    
    Eigen::VectorXd S = svd.get_singular_values();
    
    // Singular values should be in descending order
    for (int i = 0; i < S.size() - 1; ++i) {
        EXPECT_GE(S(i), S(i+1));
    }
}

TEST_F(SVDTest, ExplainedVarianceRatioSumsToOne) {
    SVD svd;
    svd.compute(X);
    
    Eigen::VectorXd ratios = svd.explained_variance_ratio();
    double sum = ratios.sum();
    
    EXPECT_NEAR(sum, 1.0, 1e-10);
}

TEST_F(SVDTest, RankEstimation) {
    // Create a rank-2 matrix (two identical rows repeated)
    Eigen::MatrixXd low_rank(4, 3);
    low_rank << 1.0, 2.0, 3.0,
                4.0, 5.0, 6.0,
                1.0, 2.0, 3.0,
                4.0, 5.0, 6.0;
    
    SVD svd;
    svd.compute(low_rank);
    
    int rank = svd.rank();
    EXPECT_EQ(rank, 2);
}

TEST_F(SVDTest, IdentityMatrix) {
    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(3, 3);
    
    SVD svd;
    svd.compute(I);
    
    Eigen::VectorXd S = svd.get_singular_values();
    
    // All singular values should be 1
    for (int i = 0; i < S.size(); ++i) {
        EXPECT_NEAR(S(i), 1.0, 1e-10);
    }
}

TEST_F(SVDTest, EmptyMatrixThrows) {
    Eigen::MatrixXd empty(0, 0);
    SVD svd;
    
    EXPECT_THROW(svd.compute(empty), std::runtime_error);
}
