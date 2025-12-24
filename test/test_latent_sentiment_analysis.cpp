#include "gtest/gtest.h"
#include "ml/latent_sentiment_analysis/latent_sentiment_analysis.h"
#include <Eigen/Dense>
#include <cmath>

using namespace Eigen;

// --- Test Fixture ---
// A test fixture helps manage common setup (like creating the object)
class LatentSentimentAnalysisTest : public ::testing::Test {
protected:
    // Parameters for the small test model
    const int TEST_K = 2; // Latent features
    const double TEST_ALPHA = 0.05; // Higher learning rate for quick convergence
    const double TEST_LAMBDA = 0.01;
    const int TEST_ITER = 500; // Fewer iterations for fast test run

    // The Document-Term Matrix (M) from our example
    // This represents a simple term-document matrix where values are term frequencies
    MatrixXd M;

    void SetUp() override {
        // Initialize the Document-Term Matrix (4 docs x 5 terms)
        M.resize(4, 5);
        M << 2, 0, 1, 1, 1, // D1
             0, 2, 1, 1, 0, // D2
             0, 1, 1, 0, 2, // D3
             1, 0, 1, 2, 1; // D4
    }

    // Helper function to create an analyzer with specific parameters
    LatentSentimentAnalysis create_analyzer() {
        return LatentSentimentAnalysis(TEST_K, TEST_ALPHA, TEST_LAMBDA, TEST_ITER);
    }
};

// --- Test Case 1: Initialization and Dimension Check ---
TEST_F(LatentSentimentAnalysisTest, InitializationAndDimensions) {
    LatentSentimentAnalysis analyzer = create_analyzer();
    analyzer.train(M);

    // 1. Check if the dimensions of U are correct (Documents x K)
    const MatrixXd& U = analyzer.get_document_features();
    ASSERT_EQ(U.rows(), M.rows()) << "U matrix should have D rows (documents)";
    ASSERT_EQ(U.cols(), TEST_K) << "U matrix should have K columns (features)";

    // 2. Check if the dimensions of V are correct (Terms x K)
    const MatrixXd& V = analyzer.get_term_features();
    ASSERT_EQ(V.rows(), M.cols()) << "V matrix should have T rows (terms)";
    ASSERT_EQ(V.cols(), TEST_K) << "V matrix should have K columns (features)";
}

// --- Test Case 2: Predict Score Sanity Check ---
TEST_F(LatentSentimentAnalysisTest, PredictScoreSanity) {
    LatentSentimentAnalysis analyzer = create_analyzer();

    // Train the model
    ::testing::internal::CaptureStdout(); 
    analyzer.train(M);
    ::testing::internal::GetCapturedStdout();

    // 1. Check predictions for observed entries are reasonable
    // The model should learn to approximate the original values
    for (int i = 0; i < M.rows(); ++i) {
        for (int j = 0; j < M.cols(); ++j) {
            if (M(i, j) > 0) {
                double prediction = analyzer.predict_score(i, j);
                // Predictions should be non-negative and roughly in the same range as input
                ASSERT_GE(prediction, -0.5) << "Prediction for observed entry should not be strongly negative";
                // After convergence, predictions should be reasonably close to actual values
                // Using generous tolerance since this is a small model with regularization
                ASSERT_NEAR(prediction, M(i, j), 2.0) << "Prediction for M[" << i << "," << j << "] should approximate " << M(i, j);
            }
        }
    }
    
    // 2. Check that predictions for unobserved entries are reasonable
    // These should generally be small (model learned sparsity)
    double unobserved_prediction = analyzer.predict_score(0, 1); // M[0,1] = 0
    ASSERT_GE(unobserved_prediction, -1.0) << "Prediction for unobserved entry should not be too negative";
    ASSERT_LE(unobserved_prediction, 3.0) << "Prediction for unobserved entry should not be too large";
}

// --- Test Case 3: Convergence Test (RMSE Reduction) ---
TEST_F(LatentSentimentAnalysisTest, ConvergenceTest) {
    // We'll run training twice: once for a short period, once for the full period, 
    // and verify that the error (RMSE) decreases with more iterations.
    
    // Analyzer for short training
    LatentSentimentAnalysis short_analyzer(TEST_K, TEST_ALPHA, TEST_LAMBDA, 10);
    // Suppress cout messages during test training for cleaner output
    ::testing::internal::CaptureStdout(); 
    short_analyzer.train(M);
    // Get rid of the captured output
    ::testing::internal::GetCapturedStdout();

    // Analyzer for full training
    LatentSentimentAnalysis full_analyzer = create_analyzer();
    ::testing::internal::CaptureStdout(); 
    full_analyzer.train(M);
    ::testing::internal::GetCapturedStdout();

    // 1. Get the final error for the short run (M - U*V^T error)
    double short_run_error = 0.0;
    const MatrixXd& U_short = short_analyzer.get_document_features();
    const MatrixXd& V_short = short_analyzer.get_term_features();
    
    // Calculate final mean squared error (MSE) over the observed entries
    int observed_count = 0;
    for (int i = 0; i < M.rows(); ++i) {
        for (int j = 0; j < M.cols(); ++j) {
            if (M(i, j) > 0) {
                observed_count++;
                double prediction = U_short.row(i).dot(V_short.row(j));
                double error = M(i, j) - prediction;
                short_run_error += error * error;
            }
        }
    }
    double short_run_rmse = std::sqrt(short_run_error / observed_count);
    
    // 2. Get the final error for the full run
    double full_run_error = 0.0;
    const MatrixXd& U_full = full_analyzer.get_document_features();
    const MatrixXd& V_full = full_analyzer.get_term_features();
    
    for (int i = 0; i < M.rows(); ++i) {
        for (int j = 0; j < M.cols(); ++j) {
            if (M(i, j) > 0) {
                double prediction = U_full.row(i).dot(V_full.row(j));
                double error = M(i, j) - prediction;
                full_run_error += error * error;
            }
        }
    }
    double full_run_rmse = std::sqrt(full_run_error / observed_count);

    // The key convergence test: RMSE of the full run must be lower than the short run
    ASSERT_LT(full_run_rmse, short_run_rmse) << "Full training run should result in a lower RMSE than the short run, indicating convergence.";
}

// --- Test Case 4: Latent Feature Learning (Matrix Reconstruction) ---
TEST_F(LatentSentimentAnalysisTest, MatrixReconstruction) {
    LatentSentimentAnalysis analyzer = create_analyzer();
    ::testing::internal::CaptureStdout(); 
    analyzer.train(M);
    ::testing::internal::GetCapturedStdout();

    const MatrixXd& U = analyzer.get_document_features();
    const MatrixXd& V = analyzer.get_term_features();
    
    // Reconstruct the matrix from the learned factors
    MatrixXd M_reconstructed = U * V.transpose();
    
    // 1. Check that observed entries are well-approximated
    double total_observed_error = 0.0;
    int observed_count = 0;
    
    for (int i = 0; i < M.rows(); ++i) {
        for (int j = 0; j < M.cols(); ++j) {
            if (M(i, j) > 0) {
                observed_count++;
                double error = M(i, j) - M_reconstructed(i, j);
                total_observed_error += error * error;
            }
        }
    }
    
    double rmse = std::sqrt(total_observed_error / observed_count);
    
    // The RMSE should be reasonably low after training
    ASSERT_LT(rmse, 0.5) << "RMSE of reconstruction should be low after training (< 0.5)";
    
    // 2. Check that the latent features have non-trivial magnitudes
    // (i.e., the model actually learned something)
    double u_norm = U.norm();
    double v_norm = V.norm();
    
    ASSERT_GT(u_norm, 0.1) << "Document features should have non-trivial magnitude";
    ASSERT_GT(v_norm, 0.1) << "Term features should have non-trivial magnitude";
    
    // 3. Verify that features are diverse (not all identical)
    // Check that columns of U and V have different values
    if (TEST_K >= 2) {
        double col_diff_u = (U.col(0) - U.col(1)).norm();
        double col_diff_v = (V.col(0) - V.col(1)).norm();
        
        ASSERT_GT(col_diff_u, 0.01) << "Document feature columns should be different";
        ASSERT_GT(col_diff_v, 0.01) << "Term feature columns should be different";
    }
}