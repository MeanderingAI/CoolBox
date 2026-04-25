
#include "mytrix_eigen_compat.hpp"
#include <iostream>
#include <kalman_filter.h>

// The constructor initializes the filter's matrices
KalmanFilter::KalmanFilter(double dt,
                           const mytrix::Matrix& A,
                           const mytrix::Matrix& C,
                           const mytrix::Matrix& Q,
                           const mytrix::Matrix& R,
                           const mytrix::Matrix& P)
    : dt(dt), A(A), C(C), Q(Q), R(R), P(P) {}

// Initializes the state with a given initial guess
void KalmanFilter::init(const mytrix::Vector& x0) {
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
void KalmanFilter::update(const mytrix::Vector& y) {
    // Calculates the Kalman Gain
    mytrix::Matrix S = C * P * C.transpose() + R;
    mytrix::Matrix K = P * C.transpose() * S.inverse();

    // Updates the state estimate
    mytrix::Vector y_hat = C * x;
    x = x + K * (y - y_hat);

    // Updates the error covariance
    mytrix::Matrix I = mytrix::Matrix::Identity(P.rows(), P.cols());
    P = (I - K * C) * P;
}

// Get the current state estimate
const Eigen::VectorXd& KalmanFilter::state() const {
    return x;
}

const Eigen::MatrixXd& KalmanFilter::covariance() const {
    return P;
}