
#include "mytrix_eigen_compat.hpp"
#include <Eigen/Dense>
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
    x = mytrix::Vector(A.data * x.data);
    Eigen::MatrixXd At = A.data.transpose();
    P = mytrix::Matrix(A.data * P.data * At + Q.data);
}

// Update step
void KalmanFilter::update(const mytrix::Vector& y) {
    Eigen::MatrixXd Ct = C.data.transpose();
    Eigen::MatrixXd S_eig = C.data * P.data * Ct + R.data;
    Eigen::MatrixXd K_eig = P.data * Ct * S_eig.inverse();

    Eigen::VectorXd y_hat = C.data * x.data;
    x = mytrix::Vector(x.data + K_eig * (y.data - y_hat));

    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(P.rows(), P.cols());
    P = mytrix::Matrix((I - K_eig * C.data) * P.data);
}

// Get the current state estimate
const mytrix::Vector& KalmanFilter::state() const {
    return x;
}

const mytrix::Matrix& KalmanFilter::covariance() const {
    return P;
}