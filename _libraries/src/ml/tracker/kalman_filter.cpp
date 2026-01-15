
#include <Eigen/Dense>
#include <iostream>
#include "ML/tracker/kalman_filter.h"

// The constructor initializes the filter's matrices
KalmanFilter::KalmanFilter(double dt,
                           const Eigen::MatrixXd& A,
                           const Eigen::MatrixXd& C,
                           const Eigen::MatrixXd& Q,
                           const Eigen::MatrixXd& R,
                           const Eigen::MatrixXd& P)
    : dt(dt), A(A), C(C), Q(Q), R(R), P(P) {}

// Initializes the state with a given initial guess
void KalmanFilter::init(const Eigen::VectorXd& x0) {
    x = x0;
}

// Predict step
void KalmanFilter::predict() {
    // Predicts the next state
    x = A * x;
    // Predicts the next error covariance
    P = A * P * A.transpose() + Q;
}

// Update step
void KalmanFilter::update(const Eigen::VectorXd& y) {
    // Calculates the Kalman Gain
    Eigen::MatrixXd S = C * P * C.transpose() + R;
    Eigen::MatrixXd K = P * C.transpose() * S.inverse();

    // Updates the state estimate
    Eigen::VectorXd y_hat = C * x;
    x = x + K * (y - y_hat);

    // Updates the error covariance
    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(P.rows(), P.cols());
    P = (I - K * C) * P;
}

// Get the current state estimate
const Eigen::VectorXd& KalmanFilter::state() const {
    return x;
}

const Eigen::MatrixXd& KalmanFilter::covariance() const {
    return P;
}