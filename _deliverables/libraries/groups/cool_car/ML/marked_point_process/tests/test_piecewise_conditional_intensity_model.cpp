#include "tyst_framework.hpp"

#include "piecewise_conditional_intensity_model.h"

#include <cmath>

TYST_TEST(PiecewiseConditionalIntensityModelTest, UsesPerIntervalConstantParameters) {
	PiecewiseConditionalIntensityModel model(2, 0.1, 5);
	model.create_uniform_intervals(0.0, 4.0, PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);

	model.set_interval_parameters(0, {0.5});
	model.set_interval_parameters(1, {1.5});

	const std::vector<double> expected_counts = model.get_expected_counts({{0.1, 1.2, 2.1, 3.2}, {0.2, 2.5}});
	const auto [aic, bic] = model.compute_information_criteria({{0.1, 1.2, 2.1, 3.2}, {0.2, 2.5}});

	TYST_EXPECT_NEAR(model.predict_intensity(1.0, {}), 0.5, 1e-12);
	TYST_EXPECT_NEAR(model.predict_intensity(3.0, {}), 1.5, 1e-12);
	TYST_EXPECT_NEAR(expected_counts[0], 1.5, 1e-12);
	TYST_EXPECT_NEAR(expected_counts[1], 1.5, 1e-12);
	TYST_EXPECT_TRUE(std::isfinite(aic));
	TYST_EXPECT_TRUE(std::isfinite(bic));
}

TYST_TEST(PiecewiseConditionalIntensityModelTest, ComputesCoxIntensityFromCovariates) {
	PiecewiseConditionalIntensityModel model(1, 0.1, 1);
	std::vector<PiecewiseConditionalIntensityModel::TimeInterval> intervals;
	intervals.emplace_back(0.0, 5.0, PiecewiseConditionalIntensityModel::IntensityType::COX);
	model.set_intervals(intervals);

	std::vector<double> parameters = {2.0, 0.5};
	std::vector<double> covariates = {1.0, 1.0};
	model.set_interval_parameters(0, parameters);

	TYST_EXPECT_NEAR(
		model.predict_intensity_with_covariates(1.0, {}, covariates),
		2.0 * std::exp(1.0),
		1e-12);
}

TYST_TEST(PiecewiseConditionalIntensityModelTest, InfersTip17SemanticEventsFromVisualWords) {
	PiecewiseConditionalIntensityModel model(2, 0.1, 1);
	model.create_uniform_intervals(0.0, 4.0, PiecewiseConditionalIntensityModel::IntensityType::CONSTANT);
	model.set_interval_parameters(0, {0.05});
	model.set_interval_parameters(1, {0.05});

	PiecewiseConditionalIntensityModel::Tip17InferenceConfig config;
	config.semantic_label_count = 2;
	config.sample_count = 80;
	config.burn_in = 10;
	config.random_seed = 7;
	config.visual_weight = 3.0;
	config.base_semantic_rate = 0.02;
	config.posterior_threshold = 0.5;
	config.visual_word_to_semantic_label = {{42, 1}};

	const auto result = model.infer_tip17_video_events({
		{0.5, 42, 1.0},
		{0.7, 42, 0.9},
		{2.0, 42, 0.8}
	}, config);

	TYST_EXPECT_TRUE(result.virtual_event_count >= 0);
	TYST_EXPECT_TRUE(!result.semantic_events.empty());
	TYST_EXPECT_TRUE(result.semantic_label_scores[1] > result.semantic_label_scores[0]);
	TYST_EXPECT_EQ(result.semantic_events.front().semantic_label, 1);
	TYST_EXPECT_TRUE(result.semantic_events.front().end_time > result.semantic_events.front().start_time);
}
