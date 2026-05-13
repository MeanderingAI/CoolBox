#include "tyst_framework.hpp"

#include "extended_kalman_filter.h"

TYST_TEST(ExtendedKalmanFilterTest, AppliesConfiguredModelsDuringPredictAndUpdate) {
	Eigen::VectorXd initial_state(1);
	initial_state << 0.0;
	Eigen::MatrixXd P0(1, 1);
	Eigen::MatrixXd Q(1, 1);
	Eigen::MatrixXd R(1, 1);
	P0 << 1.0;
	Q << 0.0;
	R << 0.1;

	ExtendedKalmanFilter filter(initial_state, P0, Q, R);
	filter.setProcessModel(
		[](const mytrix::Vector& state) -> mytrix::Vector {
			return mytrix::Vector{state[0] + 1.0};
		},
		[](const mytrix::Vector&) -> mytrix::Matrix {
			mytrix::Matrix jacobian(1, 1);
			jacobian.at(0, 0) = 1.0;
			return jacobian;
		});
	filter.setMeasurementModel(
		[](const mytrix::Vector& state) -> mytrix::Vector {
			return mytrix::Vector{state[0]};
		},
		[](const mytrix::Vector&) -> mytrix::Matrix {
			mytrix::Matrix jacobian(1, 1);
			jacobian.at(0, 0) = 1.0;
			return jacobian;
		});

	filter.predict();

	Eigen::VectorXd measurement(1);
	measurement << 1.2;
	filter.update(measurement);

	TYST_EXPECT_NEAR(filter.state()(0), 1.1818181818181819, 1e-12);
	TYST_EXPECT_NEAR(filter.covariance()(0, 0), 1.0 / 11.0, 1e-12);
}
