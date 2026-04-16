﻿#include "tyst_framework.hpp"

#include "kalman_filter.h"

TYST_TEST(KalmanFilterTest, UpdatesLinearStateTowardMeasurement) {
	Eigen::MatrixXd A(1, 1);
	Eigen::MatrixXd C(1, 1);
	Eigen::MatrixXd Q(1, 1);
	Eigen::MatrixXd R(1, 1);
	Eigen::MatrixXd P(1, 1);
	A << 1.0;
	C << 1.0;
	Q << 0.0;
	R << 0.1;
	P << 1.0;

	KalmanFilter filter(1.0, A, C, Q, R, P);
	Eigen::VectorXd initial_state(1);
	initial_state << 0.0;
	filter.init(initial_state);

	filter.predict();

	Eigen::VectorXd measurement(1);
	measurement << 1.0;
	filter.update(measurement);

	TYST_EXPECT_NEAR(filter.state()(0), 1.0 / 1.1, 1e-12);
	TYST_EXPECT_NEAR(filter.covariance()(0, 0), 1.0 / 11.0, 1e-12);
}
