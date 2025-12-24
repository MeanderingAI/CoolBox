
#include <functional>
#include "ml/tracker/extended_kalman_filter.h"

ExtendedKalmanFilter::ExtendedKalmanFilter(
    const Eigen::VectorXd& x0,
    const Eigen::MatrixXd& P0,
    const Eigen::MatrixXd& Q,
    const Eigen::MatrixXd& R
)
    : x_(x0), P_(P0), Q_(Q), R_(R)
{}

void ExtendedKalmanFilter::setProcessModel(
    const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& f,
    const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& F
) {
    f_ = f;
    F_ = F;
}

void ExtendedKalmanFilter::setMeasurementModel(
    const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& h,
    const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& H
) {
    h_ = h;
    H_ = H;
}

void ExtendedKalmanFilter::predict() {
    if (!f_ || !F_) return; // Optionally throw or assert
    x_ = f_(x_);
    Eigen::MatrixXd Fk = F_(x_);
    P_ = Fk * P_ * Fk.transpose() + Q_;
}

void ExtendedKalmanFilter::update(const Eigen::VectorXd& z) {
    if (!h_ || !H_) return; // Optionally throw or assert
    Eigen::VectorXd y = z - h_(x_);
    Eigen::MatrixXd Hk = H_(x_);
    Eigen::MatrixXd S = Hk * P_ * Hk.transpose() + R_;
    Eigen::MatrixXd K = P_ * Hk.transpose() * S.inverse();
    x_ = x_ + K * y;
    P_ = (Eigen::MatrixXd::Identity(x_.size(), x_.size()) - K * Hk) * P_;
}

const Eigen::VectorXd& ExtendedKalmanFilter::state() const {
    return x_;
}

const Eigen::MatrixXd& ExtendedKalmanFilter::covariance() const {
    return P_;
}