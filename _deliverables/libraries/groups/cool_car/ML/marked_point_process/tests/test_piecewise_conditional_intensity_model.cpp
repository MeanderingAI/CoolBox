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
