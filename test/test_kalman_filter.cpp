#include <gtest/gtest.h>
#include <Eigen/Dense>
#include "ml/tracker/kalman_filter.h"

TEST(KalmanFilterTest, LinearPrediction) {
    int n = 2, m = 1;
    double dt = 1.0;
    Eigen::MatrixXd A(n, n); A << 1, dt, 0, 1;
    Eigen::MatrixXd C(m, n); C << 1, 0;
    Eigen::MatrixXd Q = 0.001 * Eigen::MatrixXd::Identity(n, n);
    Eigen::MatrixXd R = 0.1 * Eigen::MatrixXd::Identity(m, m);
    Eigen::MatrixXd P = Eigen::MatrixXd::Identity(n, n);
    Eigen::VectorXd x0(n); x0 << 0, 1;

    KalmanFilter kf(dt, A, C, Q, R, P);
    kf.init(x0);
    kf.predict();
    Eigen::VectorXd x_pred = kf.state();
    EXPECT_NEAR(x_pred(0), 1.0, 1e-6);
    EXPECT_NEAR(x_pred(1), 1.0, 1e-6);
}

