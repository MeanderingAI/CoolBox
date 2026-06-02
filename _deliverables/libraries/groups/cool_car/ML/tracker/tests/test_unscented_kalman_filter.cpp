#include "tyst_framework.hpp"

#include "unscented_kalman_filter.h"

#include <cmath>

TYST_TEST(UnscentedKalmanFilterTest, HandlesSimpleLinearModels) {
	UnscentedKalmanFilter filter(1, 1);

	Eigen::VectorXd initial_state(1);
	initial_state << 0.0;
	Eigen::MatrixXd initial_covariance(1, 1);
	initial_covariance << 1.0;
	Eigen::MatrixXd process_noise(1, 1);
	process_noise << 0.01;
	Eigen::MatrixXd measurement_noise(1, 1);
	measurement_noise << 0.1;
	filter.initialize(initial_state, initial_covariance);
	filter.setProcessModel(
		[](const mytrix::Vector& state) -> mytrix::Vector {
			return mytrix::Vector(std::vector<double>{state.at(0) + 1.0}, 1);
		},
		process_noise);
	filter.setMeasurementModel(
		[](const mytrix::Vector& state) -> mytrix::Vector {
			return mytrix::Vector(std::vector<double>{state.at(0)}, 1);
		},
		measurement_noise);

	filter.predict();

	Eigen::VectorXd measurement(1);
	measurement << 1.2;
	filter.update(measurement);

	TYST_EXPECT_TRUE(std::isfinite(filter.state()(0)));
	TYST_EXPECT_GT(filter.state()(0), 1.0);
	TYST_EXPECT_LT(filter.state()(0), 1.2);
	TYST_EXPECT_GT(filter.covariance()(0, 0), 0.0);
}
