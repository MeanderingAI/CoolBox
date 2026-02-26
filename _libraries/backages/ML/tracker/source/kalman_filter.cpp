#include "kalman_filter.h"

KalmanFilter::KalmanFilter(double dt, const Eigen::MatrixXd& A, const Eigen::MatrixXd& C,
                           const Eigen::MatrixXd& Q, const Eigen::MatrixXd& R,
                           const Eigen::MatrixXd& P)
    : dt(dt), A(A), C(C), Q(Q), R(R), P(P) {
    int n = A.rows();
    x = Eigen::VectorXd::Zero(n);
}

void KalmanFilter::init(const Eigen::VectorXd& x0) {
    x = x0;
}

void KalmanFilter::predict() {
    x = A * x;
    P = A * P * A.transpose() + Q;
}

void KalmanFilter::update(const Eigen::VectorXd& y) {
    Eigen::MatrixXd S = C * P * C.transpose() + R;
    Eigen::MatrixXd K = P * C.transpose() * S.inverse();
    x = x + K * (y - C * x);
    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(x.size(), x.size());
    P = (I - K * C) * P;
}

const Eigen::VectorXd& KalmanFilter::state() const {
    return x;
}

const Eigen::MatrixXd& KalmanFilter::covariance() const {
    return P;
}
