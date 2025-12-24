#include <gtest/gtest.h>
#include <Eigen/Dense>
#include "ml/tracker/unscented_kalman_filter.h"

TEST(UnscentedKalmanFilterTest, NonlinearPrediction) {
    int n = 2, m = 1;
    double dt = 1.0;
    Eigen::VectorXd x0(n); x0 << 0, 1;
    Eigen::MatrixXd P0 = Eigen::MatrixXd::Identity(n, n);
    Eigen::MatrixXd Q = 0.001 * Eigen::MatrixXd::Identity(n, n);
    Eigen::MatrixXd R = 0.1 * Eigen::MatrixXd::Identity(m, m);

    auto f = [dt](const Eigen::VectorXd& x) {
        Eigen::VectorXd x_new(2);
        x_new(0) = x(0) + x(1) * dt;
        x_new(1) = x(1);
        return x_new;
    };
    auto h = [](const Eigen::VectorXd& x) {
        Eigen::VectorXd z(1);
        z(0) = std::sqrt(x(0)*x(0) + 1.0);
        return z;
    };

    UnscentedKalmanFilter ukf(n, m);
    ukf.initialize(x0, P0);
    ukf.setProcessModel(f, Q);
    ukf.setMeasurementModel(h, R);
    ukf.predict();
    Eigen::VectorXd x_pred = ukf.state();
    EXPECT_NEAR(x_pred(0), 1.0, 1e-6);
    EXPECT_NEAR(x_pred(1), 1.0, 1e-6);
}