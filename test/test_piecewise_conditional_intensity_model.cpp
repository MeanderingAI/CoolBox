#include "gtest/gtest.h"
#include <piecewise_conditional_intensity_model.h>
#include <Eigen/Dense>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace Eigen;

// --- Test Fixture ---
class PiecewiseConditionalIntensityModelTest : public ::testing::Test {
protected:
    const int TEST_NUM_INTERVALS = 3;
    const double TEST_LEARNING_RATE = 0.05;
    const int TEST_ITERATIONS = 100;

    // Sample training data
    std::vector<std::vector<double>> event_sequences;

    void SetUp() override {
        // Create synthetic sequences with varying intensity
        // Sequence 1: Events spread over time
        event_sequences.push_back({0.5, 1.2, 2.3, 3.1, 4.5, 5.2, 6.8, 7.5, 8.9, 9.2});
        
        // Sequence 2: More clustered events
        event_sequences.push_back({0.8, 1.0, 1.5, 4.2, 4.5, 5.0, 8.1, 8.3, 8.7, 9.5});
    }

    PiecewiseConditionalIntensityModel create_model() {
        return PiecewiseConditionalIntensityModel(
            TEST_NUM_INTERVALS, TEST_LEARNING_RATE, TEST_ITERATIONS);
    }
};

// --- Test Case 1: Initialization ---
TEST_F(PiecewiseConditionalIntensityModelTest, Initialization) {
    PiecewiseConditionalIntensityModel pcim = create_model();

    // Model should be initialized but intervals not yet defined
    auto intervals = pcim.get_intervals();
    ASSERT_EQ(intervals.size(), 0) << "Intervals should be empty before creation";
}

// --- Test Case 2: Create Uniform Intervals ---
TEST_F(PiecewiseConditionalIntensityModelTest, CreateUniformIntervals) {
    PiecewiseConditionalIntensityModel pcim = create_model();

    double time_min = 0.0;
    double time_max = 10.0;
    
    pcim.create_uniform_intervals(
        time_min, time_max, 
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    auto intervals = pcim.get_intervals();
    
    ASSERT_EQ(intervals.size(), TEST_NUM_INTERVALS) 
        << "Should create correct number of intervals";
    
    // Check that intervals partition the time domain
    ASSERT_NEAR(intervals[0].start_time, time_min, 1e-6) 
        << "First interval should start at time_min";
    ASSERT_NEAR(intervals[TEST_NUM_INTERVALS - 1].end_time, time_max, 1e-6) 
        << "Last interval should end at time_max";
    
    // Check intervals are contiguous
    for (size_t i = 1; i < intervals.size(); ++i) {
        ASSERT_NEAR(intervals[i-1].end_time, intervals[i].start_time, 1e-6)
            << "Intervals should be contiguous";
    }
    
    // Check all have same intensity type
    for (const auto& interval : intervals) {
        ASSERT_EQ(interval.intensity_type, 
                  PiecewiseConditionalIntensityModel::IntensityType::CONSTANT)
            << "All intervals should have specified intensity type";
    }
}

// --- Test Case 3: Create Adaptive Intervals ---
TEST_F(PiecewiseConditionalIntensityModelTest, CreateAdaptiveIntervals) {
    PiecewiseConditionalIntensityModel pcim = create_model();

    // Flatten all events
    std::vector<double> all_events;
    for (const auto& seq : event_sequences) {
        all_events.insert(all_events.end(), seq.begin(), seq.end());
    }

    pcim.create_adaptive_intervals(
        all_events,
        PiecewiseConditionalIntensityModel::IntensityType::HAWKES);

    auto intervals = pcim.get_intervals();
    
    ASSERT_EQ(intervals.size(), TEST_NUM_INTERVALS) 
        << "Should create correct number of intervals";
    
    // Check intervals cover the data range
    double min_time = *std::min_element(all_events.begin(), all_events.end());
    double max_time = *std::max_element(all_events.begin(), all_events.end());
    
    ASSERT_LE(intervals[0].start_time, min_time) 
        << "First interval should start before or at minimum time";
    ASSERT_GE(intervals[TEST_NUM_INTERVALS - 1].end_time, max_time) 
        << "Last interval should end after or at maximum time";
}

// --- Test Case 4: Set and Get Interval Parameters ---
TEST_F(PiecewiseConditionalIntensityModelTest, IntervalParameters) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0, 
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Set custom parameters for first interval
    VectorXd params(1);
    params << 0.5;
    pcim.set_interval_parameters(0, params);
    
    VectorXd retrieved = pcim.get_interval_parameters(0);
    ASSERT_TRUE(retrieved.isApprox(params, 1e-6)) 
        << "Retrieved parameters should match set parameters";
}

// --- Test Case 5: Training Convergence ---
TEST_F(PiecewiseConditionalIntensityModelTest, TrainingConvergence) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Capture output
    ::testing::internal::CaptureStdout();
    
    double initial_ll = pcim.log_likelihood(event_sequences);
    
    pcim.fit(event_sequences);
    
    double final_ll = pcim.log_likelihood(event_sequences);
    
    ::testing::internal::GetCapturedStdout();

    // Training should improve log-likelihood
    ASSERT_GT(final_ll, initial_ll) 
        << "Training should increase log-likelihood";
}

// --- Test Case 6: Predict Intensity ---
TEST_F(PiecewiseConditionalIntensityModelTest, PredictIntensity) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Set known constant intensity for first interval
    VectorXd params(1);
    params << 0.8;
    pcim.set_interval_parameters(0, params);

    // Predict intensity in first interval
    std::vector<double> empty_history;
    double intensity = pcim.predict_intensity(1.5, empty_history);
    
    ASSERT_NEAR(intensity, 0.8, 1e-6) 
        << "For constant intensity, prediction should equal parameter";
}

// --- Test Case 7: Hawkes Intensity Type ---
TEST_F(PiecewiseConditionalIntensityModelTest, HawkesIntensity) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::HAWKES);

    // Train the model
    ::testing::internal::CaptureStdout();
    pcim.fit(event_sequences);
    ::testing::internal::GetCapturedStdout();

    // Test intensity with and without history
    std::vector<double> empty_history;
    double intensity_no_history = pcim.predict_intensity(5.0, empty_history);
    
    std::vector<double> with_history = {4.5, 4.8};
    double intensity_with_history = pcim.predict_intensity(5.0, with_history);
    
    // With recent history, intensity should be higher due to self-excitation
    ASSERT_GT(intensity_with_history, intensity_no_history) 
        << "Hawkes process should show self-excitation with recent events";
}

// --- Test Case 8: Generate Sequence ---
TEST_F(PiecewiseConditionalIntensityModelTest, GenerateSequence) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Set reasonable parameters
    for (int i = 0; i < TEST_NUM_INTERVALS; ++i) {
        VectorXd params(1);
        params << 0.5 + i * 0.1;  // Varying intensity across intervals
        pcim.set_interval_parameters(i, params);
    }

    double time_horizon = 10.0;
    std::vector<double> generated = pcim.generate_sequence(time_horizon, 100);

    // Check generated sequence properties
    ASSERT_GT(generated.size(), 0) << "Should generate at least some events";
    
    // All times should be within horizon
    for (double t : generated) {
        ASSERT_GE(t, 0.0) << "Event times should be non-negative";
        ASSERT_LE(t, time_horizon) << "Event times should be within horizon";
    }
    
    // Times should be sorted
    for (size_t i = 1; i < generated.size(); ++i) {
        ASSERT_GE(generated[i], generated[i-1]) 
            << "Generated event times should be non-decreasing";
    }
}

// --- Test Case 9: Log-Likelihood Computation ---
TEST_F(PiecewiseConditionalIntensityModelTest, LogLikelihood) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Set parameters
    for (int i = 0; i < TEST_NUM_INTERVALS; ++i) {
        VectorXd params(1);
        params << 0.5;
        pcim.set_interval_parameters(i, params);
    }

    double ll = pcim.log_likelihood(event_sequences);
    
    ASSERT_TRUE(std::isfinite(ll)) << "Log-likelihood should be finite";
    ASSERT_LT(ll, 0.0) << "Log-likelihood should typically be negative";
}

// --- Test Case 10: Expected Counts ---
TEST_F(PiecewiseConditionalIntensityModelTest, ExpectedCounts) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Train model
    ::testing::internal::CaptureStdout();
    pcim.fit(event_sequences);
    ::testing::internal::GetCapturedStdout();

    VectorXd expected_counts = pcim.get_expected_counts(event_sequences);
    
    ASSERT_EQ(expected_counts.size(), TEST_NUM_INTERVALS) 
        << "Expected counts should be computed for each interval";
    
    // All expected counts should be positive
    for (int i = 0; i < expected_counts.size(); ++i) {
        ASSERT_GT(expected_counts(i), 0.0) 
            << "Expected counts should be positive";
    }
}

// --- Test Case 11: Information Criteria ---
TEST_F(PiecewiseConditionalIntensityModelTest, InformationCriteria) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Train model
    ::testing::internal::CaptureStdout();
    pcim.fit(event_sequences);
    ::testing::internal::GetCapturedStdout();

    auto [aic, bic] = pcim.compute_information_criteria(event_sequences);
    
    ASSERT_TRUE(std::isfinite(aic)) << "AIC should be finite";
    ASSERT_TRUE(std::isfinite(bic)) << "BIC should be finite";
    
    // BIC typically penalizes more than AIC for same number of parameters
    ASSERT_GT(bic, aic) << "BIC should typically be greater than AIC";
}

// --- Test Case 12: Multiple Intensity Types ---
TEST_F(PiecewiseConditionalIntensityModelTest, MultipleIntensityTypes) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    // Create intervals with different intensity types
    std::vector<PiecewiseConditionalIntensityModel::TimeInterval> custom_intervals;
    
    custom_intervals.emplace_back(
        0.0, 3.0, PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);
    custom_intervals.emplace_back(
        3.0, 7.0, PiecewiseConditionalIntensityModel::IntensityType::LINEAR);
    custom_intervals.emplace_back(
        7.0, 10.0, PiecewiseConditionalIntensityModel::IntensityType::EXPONENTIAL);
    
    pcim.set_intervals(custom_intervals);
    
    auto intervals = pcim.get_intervals();
    ASSERT_EQ(intervals.size(), 3) << "Should have 3 custom intervals";
    
    // Verify different intensity types
    ASSERT_EQ(intervals[0].intensity_type, 
              PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);
    ASSERT_EQ(intervals[1].intensity_type, 
              PiecewiseConditionalIntensityModel::IntensityType::LINEAR);
    ASSERT_EQ(intervals[2].intensity_type, 
              PiecewiseConditionalIntensityModel::IntensityType::EXPONENTIAL);
}

// --- Test Case 13: Interval Boundary Handling ---
TEST_F(PiecewiseConditionalIntensityModelTest, IntervalBoundaries) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Set different intensities for each interval
    for (int i = 0; i < TEST_NUM_INTERVALS; ++i) {
        VectorXd params(1);
        params << 1.0 + i * 0.5;
        pcim.set_interval_parameters(i, params);
    }

    auto intervals = pcim.get_intervals();
    
    std::vector<double> history;
    
    // Test intensity at interval boundaries
    double intensity_start = pcim.predict_intensity(intervals[0].start_time, history);
    double intensity_boundary = pcim.predict_intensity(intervals[0].end_time, history);
    double intensity_next = pcim.predict_intensity(intervals[1].start_time, history);
    
    // At boundary, we should transition between intervals
    ASSERT_TRUE(std::isfinite(intensity_start)) 
        << "Intensity at interval start should be finite";
    ASSERT_TRUE(std::isfinite(intensity_boundary)) 
        << "Intensity at interval boundary should be finite";
}

// --- Test Case 14: Empty Sequence Handling ---
TEST_F(PiecewiseConditionalIntensityModelTest, EmptySequenceHandling) {
    PiecewiseConditionalIntensityModel pcim = create_model();
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

    // Add empty sequence
    std::vector<std::vector<double>> sequences_with_empty = event_sequences;
    sequences_with_empty.push_back(std::vector<double>());

    // Should not crash with empty sequences
    ::testing::internal::CaptureStdout();
    ASSERT_NO_THROW(pcim.fit(sequences_with_empty));
    ::testing::internal::GetCapturedStdout();
}

// --- Test Case 15: Linear Intensity Growth ---
TEST_F(PiecewiseConditionalIntensityModelTest, LinearIntensityGrowth) {
    PiecewiseConditionalIntensityModel pcim(1, TEST_LEARNING_RATE, TEST_ITERATIONS);
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::LINEAR);

    // Set linear parameters: a + b*t
    VectorXd params(2);
    params << 0.5, 0.1;  // intensity = 0.5 + 0.1*t
    pcim.set_interval_parameters(0, params);

    std::vector<double> history;
    
    // Test that intensity grows linearly
    double intensity_at_0 = pcim.predict_intensity(0.0, history);
    double intensity_at_5 = pcim.predict_intensity(5.0, history);
    double intensity_at_10 = pcim.predict_intensity(10.0, history);
    
    ASSERT_NEAR(intensity_at_0, 0.5, 1e-6) << "At t=0, intensity should be a";
    ASSERT_NEAR(intensity_at_5, 1.0, 1e-6) << "At t=5, intensity should be a + 5b";
    ASSERT_NEAR(intensity_at_10, 1.5, 1e-6) << "At t=10, intensity should be a + 10b";
}

// --- Test Case 16: Exponential Decay ---
TEST_F(PiecewiseConditionalIntensityModelTest, ExponentialDecay) {
    PiecewiseConditionalIntensityModel pcim(1, TEST_LEARNING_RATE, TEST_ITERATIONS);
    
    pcim.create_uniform_intervals(
        0.0, 10.0,
        PiecewiseConditionalIntensityModel::IntensityType::EXPONENTIAL);

    // Set exponential parameters: a*exp(-b*t)
    VectorXd params(2);
    params << 2.0, 0.5;
    pcim.set_interval_parameters(0, params);

    std::vector<double> history;
    
    double intensity_at_0 = pcim.predict_intensity(0.0, history);
    double intensity_at_2 = pcim.predict_intensity(2.0, history);
    
    // Intensity should decay exponentially
    ASSERT_NEAR(intensity_at_0, 2.0, 1e-6) << "At t=0, intensity should be a";
    ASSERT_GT(intensity_at_0, intensity_at_2) << "Intensity should decay over time";
    
    double expected_at_2 = 2.0 * std::exp(-0.5 * 2.0);
    ASSERT_NEAR(intensity_at_2, expected_at_2, 1e-6) 
        << "Exponential decay should follow formula";
}
