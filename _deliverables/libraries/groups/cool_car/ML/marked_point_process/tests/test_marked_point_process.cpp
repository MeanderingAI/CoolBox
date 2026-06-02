#include "tyst_framework.hpp"

#include "marked_point_process.h"

#include <algorithm>
#include <cmath>

using namespace ml;

TYST_TEST(MarkedPointProcessTest, UsesConfiguredParametersForIntensityPredictions) {
	MarkedPointProcess process(2, 0.01, 1);

	process.set_base_intensity({0.2, 0.3});
	mytrix::DenseMatrix excitation(2, 2);
	excitation.at(0,0) = 0.5; excitation.at(0,1) = 0.0;
	excitation.at(1,0) = 0.0; excitation.at(1,1) = 0.25;
	process.set_excitation_matrix(excitation);
	process.set_decay_rate(1.0);

	const std::vector<double> intensities = process.predict_intensity(2.0, {1.0}, {0});

	TYST_EXPECT_NEAR(intensities[0], 0.2 + 0.5 * std::exp(-1.0), 1e-12);
	TYST_EXPECT_NEAR(intensities[1], 0.3, 1e-12);
}

TYST_TEST(MarkedPointProcessTest, GeneratesValidSequencesAndFiniteLikelihoods) {
	MarkedPointProcess process(2, 0.01, 1);

	process.set_base_intensity({0.4, 0.2});
	process.set_excitation_matrix(mytrix::DenseMatrix(2, 2));
	process.set_decay_rate(1.0);

	const auto [times, marks] = process.generate_sequence(3.0, 20);
	const double log_likelihood = process.log_likelihood({{0.5, 1.5, 2.0}}, {{0, 1, 0}});

	TYST_EXPECT_EQ(times.size(), marks.size());
	TYST_EXPECT_TRUE(std::is_sorted(times.begin(), times.end()));
	for (int mark : marks) {
		TYST_EXPECT_GE(mark, 0);
		TYST_EXPECT_LT(mark, 2);
	}
	TYST_EXPECT_TRUE(std::isfinite(log_likelihood));
}
