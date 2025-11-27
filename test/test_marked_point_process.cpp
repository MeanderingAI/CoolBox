#include "gtest/gtest.h"
#include <marked_point_process.h>
#include <Eigen/Dense>
#include <cmath>
#include <vector>

using namespace Eigen;

// --- Test Fixture ---
class MarkedPointProcessTest : public ::testing::Test {
protected:
    const int TEST_NUM_MARKS = 2;
    const double TEST_LEARNING_RATE = 0.05;
    const int TEST_ITERATIONS = 100;

    // Sample training data
    std::vector<std::vector<double>> event_times;
    std::vector<std::vector<int>> event_marks;

    void SetUp() override {
        // Create simple synthetic data
        // Sequence 1: Alternating marks
        event_times.push_back({0.5, 1.0, 1.5, 2.0, 2.5, 3.0});
        event_marks.push_back({0, 1, 0, 1, 0, 1});

        // Sequence 2: More mark 0 early, more mark 1 later
        event_times.push_back({0.2, 0.4, 0.6, 2.1, 2.3, 2.5});
        event_marks.push_back({0, 0, 0, 1, 1, 1});
    }

    MarkedPointProcess create_model() {
        return MarkedPointProcess(TEST_NUM_MARKS, TEST_LEARNING_RATE, TEST_ITERATIONS);
    }
};

// --- Test Case 1: Initialization ---
TEST_F(MarkedPointProcessTest, Initialization) {
    MarkedPointProcess mpp = create_model();

    // Check that base intensity is initialized
    VectorXd mu = mpp.get_base_intensity();
    ASSERT_EQ(mu.size(), TEST_NUM_MARKS) << "Base intensity should have size equal to num_marks";
    
    // Check all values are positive
    for (int i = 0; i < mu.size(); ++i) {
        ASSERT_GT(mu(i), 0.0) << "Base intensity values should be positive";
    }

    // Check excitation matrix dimensions
    MatrixXd alpha = mpp.get_excitation_matrix();
    ASSERT_EQ(alpha.rows(), TEST_NUM_MARKS) << "Excitation matrix should be num_marks x num_marks";
    ASSERT_EQ(alpha.cols(), TEST_NUM_MARKS) << "Excitation matrix should be num_marks x num_marks";

    // Check decay rate is positive
    double beta = mpp.get_decay_rate();
    ASSERT_GT(beta, 0.0) << "Decay rate should be positive";
}

// --- Test Case 2: Parameter Setters and Getters ---
TEST_F(MarkedPointProcessTest, ParameterSettersGetters) {
    MarkedPointProcess mpp = create_model();

    // Set custom base intensity
    VectorXd custom_mu(TEST_NUM_MARKS);
    custom_mu << 0.5, 0.8;
    mpp.set_base_intensity(custom_mu);
    
    VectorXd retrieved_mu = mpp.get_base_intensity();
    ASSERT_TRUE(retrieved_mu.isApprox(custom_mu, 1e-6)) << "Retrieved base intensity should match set values";

    // Set custom excitation matrix
    MatrixXd custom_alpha(TEST_NUM_MARKS, TEST_NUM_MARKS);
    custom_alpha << 0.3, 0.2,
                    0.1, 0.4;
    mpp.set_excitation_matrix(custom_alpha);
    
    MatrixXd retrieved_alpha = mpp.get_excitation_matrix();
    ASSERT_TRUE(retrieved_alpha.isApprox(custom_alpha, 1e-6)) << "Retrieved excitation matrix should match set values";

    // Set custom decay rate
    double custom_beta = 1.5;
    mpp.set_decay_rate(custom_beta);
    
    double retrieved_beta = mpp.get_decay_rate();
    ASSERT_NEAR(retrieved_beta, custom_beta, 1e-6) << "Retrieved decay rate should match set value";
}

// --- Test Case 3: Training Convergence ---
TEST_F(MarkedPointProcessTest, TrainingConvergence) {
    MarkedPointProcess mpp = create_model();

    // Capture output to avoid cluttering test results
    ::testing::internal::CaptureStdout();
    
    // Compute initial log-likelihood
    double initial_ll = mpp.log_likelihood(event_times, event_marks);
    
    // Train the model
    mpp.fit(event_times, event_marks);
    
    // Compute final log-likelihood
    double final_ll = mpp.log_likelihood(event_times, event_marks);
    
    ::testing::internal::GetCapturedStdout();

    // Training should improve log-likelihood
    ASSERT_GT(final_ll, initial_ll) << "Training should increase log-likelihood";
}

// --- Test Case 4: Predict Intensity ---
TEST_F(MarkedPointProcessTest, PredictIntensity) {
    MarkedPointProcess mpp = create_model();

    // Set known parameters
    VectorXd mu(TEST_NUM_MARKS);
    mu << 0.5, 0.3;
    mpp.set_base_intensity(mu);

    MatrixXd alpha(TEST_NUM_MARKS, TEST_NUM_MARKS);
    alpha << 0.3, 0.1,
             0.2, 0.4;
    mpp.set_excitation_matrix(alpha);

    mpp.set_decay_rate(1.0);

    // Test with empty history - should return base intensity
    std::vector<double> empty_times;
    std::vector<int> empty_marks;
    VectorXd intensities_empty = mpp.predict_intensity(1.0, empty_times, empty_marks);
    
    ASSERT_TRUE(intensities_empty.isApprox(mu, 1e-6)) 
        << "With empty history, intensity should equal base intensity";

    // Test with some history
    std::vector<double> history_times = {0.5};
    std::vector<int> history_marks = {0};
    VectorXd intensities_with_history = mpp.predict_intensity(1.0, history_times, history_marks);
    
    // Should be greater than base intensity due to excitation
    ASSERT_GT(intensities_with_history(0), mu(0)) 
        << "Intensity with history should be higher than base intensity for self-excitation";
    ASSERT_GT(intensities_with_history(1), mu(1)) 
        << "Cross-excitation should also increase intensity for other marks";
}

// --- Test Case 5: Generate Sequence ---
TEST_F(MarkedPointProcessTest, GenerateSequence) {
    MarkedPointProcess mpp = create_model();

    // Set reasonable parameters
    VectorXd mu(TEST_NUM_MARKS);
    mu << 0.5, 0.3;
    mpp.set_base_intensity(mu);

    MatrixXd alpha(TEST_NUM_MARKS, TEST_NUM_MARKS);
    alpha << 0.2, 0.1,
             0.1, 0.3;
    mpp.set_excitation_matrix(alpha);

    mpp.set_decay_rate(1.0);

    // Generate a sequence
    double time_horizon = 10.0;
    auto [times, marks] = mpp.generate_sequence(time_horizon, 100);

    // Check that sequence is valid
    ASSERT_EQ(times.size(), marks.size()) << "Times and marks should have equal length";
    
    // Check all times are within horizon
    for (double t : times) {
        ASSERT_GE(t, 0.0) << "Event times should be non-negative";
        ASSERT_LE(t, time_horizon) << "Event times should be within time horizon";
    }

    // Check all marks are valid
    for (int m : marks) {
        ASSERT_GE(m, 0) << "Marks should be non-negative";
        ASSERT_LT(m, TEST_NUM_MARKS) << "Marks should be less than num_marks";
    }

    // Check times are sorted
    for (size_t i = 1; i < times.size(); ++i) {
        ASSERT_GE(times[i], times[i-1]) << "Event times should be non-decreasing";
    }
}

// --- Test Case 6: Log-Likelihood Computation ---
TEST_F(MarkedPointProcessTest, LogLikelihood) {
    MarkedPointProcess mpp = create_model();

    // Set parameters
    VectorXd mu(TEST_NUM_MARKS);
    mu << 0.5, 0.3;
    mpp.set_base_intensity(mu);

    MatrixXd alpha(TEST_NUM_MARKS, TEST_NUM_MARKS);
    alpha << 0.3, 0.1,
             0.2, 0.4;
    mpp.set_excitation_matrix(alpha);

    mpp.set_decay_rate(1.0);

    // Compute log-likelihood
    double ll = mpp.log_likelihood(event_times, event_marks);

    // Log-likelihood should be finite (not NaN or Inf)
    ASSERT_TRUE(std::isfinite(ll)) << "Log-likelihood should be finite";
    
    // For valid parameters and data, log-likelihood should be negative but not too extreme
    ASSERT_LT(ll, 0.0) << "Log-likelihood should typically be negative";
    ASSERT_GT(ll, -1000.0) << "Log-likelihood should not be extremely negative for reasonable data";
}

// --- Test Case 7: Parameter Validation ---
TEST_F(MarkedPointProcessTest, ParameterValidation) {
    MarkedPointProcess mpp = create_model();

    // Test invalid base intensity size
    VectorXd bad_mu(TEST_NUM_MARKS + 1);
    bad_mu.setConstant(0.5);
    ASSERT_THROW(mpp.set_base_intensity(bad_mu), std::invalid_argument) 
        << "Should throw exception for incorrect base intensity size";

    // Test invalid excitation matrix size
    MatrixXd bad_alpha(TEST_NUM_MARKS + 1, TEST_NUM_MARKS);
    bad_alpha.setConstant(0.3);
    ASSERT_THROW(mpp.set_excitation_matrix(bad_alpha), std::invalid_argument)
        << "Should throw exception for incorrect excitation matrix dimensions";

    // Test invalid decay rate
    ASSERT_THROW(mpp.set_decay_rate(-1.0), std::invalid_argument)
        << "Should throw exception for negative decay rate";
    ASSERT_THROW(mpp.set_decay_rate(0.0), std::invalid_argument)
        << "Should throw exception for zero decay rate";
}

// --- Test Case 8: Self-Excitation Property ---
TEST_F(MarkedPointProcessTest, SelfExcitationProperty) {
    MarkedPointProcess mpp = create_model();

    // Set up parameters with strong self-excitation
    VectorXd mu(TEST_NUM_MARKS);
    mu << 0.1, 0.1;
    mpp.set_base_intensity(mu);

    MatrixXd alpha(TEST_NUM_MARKS, TEST_NUM_MARKS);
    alpha << 0.5, 0.05,  // Strong self-excitation for mark 0
             0.05, 0.5;  // Strong self-excitation for mark 1
    mpp.set_excitation_matrix(alpha);

    mpp.set_decay_rate(1.0);

    // Recent event of mark 0 should strongly increase intensity of mark 0
    std::vector<double> history_times = {0.9};  // Recent event
    std::vector<int> history_marks = {0};
    
    VectorXd intensities = mpp.predict_intensity(1.0, history_times, history_marks);
    
    // Self-excitation should dominate
    ASSERT_GT(intensities(0), intensities(1)) 
        << "Recent mark 0 event should increase mark 0 intensity more than mark 1";
}

// --- Test Case 9: Temporal Decay ---
TEST_F(MarkedPointProcessTest, TemporalDecay) {
    MarkedPointProcess mpp = create_model();

    VectorXd mu(TEST_NUM_MARKS);
    mu << 0.1, 0.1;
    mpp.set_base_intensity(mu);

    MatrixXd alpha(TEST_NUM_MARKS, TEST_NUM_MARKS);
    alpha << 0.5, 0.1,
             0.1, 0.5;
    mpp.set_excitation_matrix(alpha);

    mpp.set_decay_rate(2.0);  // Fast decay

    std::vector<double> history_times = {0.0};
    std::vector<int> history_marks = {0};

    // Intensity should decrease as we move further from the event
    VectorXd intensity_at_0_5 = mpp.predict_intensity(0.5, history_times, history_marks);
    VectorXd intensity_at_1_0 = mpp.predict_intensity(1.0, history_times, history_marks);
    VectorXd intensity_at_2_0 = mpp.predict_intensity(2.0, history_times, history_marks);

    // Intensity should decay over time
    ASSERT_GT(intensity_at_0_5(0), intensity_at_1_0(0)) 
        << "Intensity should decay as time increases";
    ASSERT_GT(intensity_at_1_0(0), intensity_at_2_0(0)) 
        << "Intensity should continue to decay";
    
    // But should approach base intensity, not go below it
    ASSERT_GT(intensity_at_2_0(0), mu(0) * 0.9) 
        << "Intensity should approach base intensity but not go significantly below";
}
