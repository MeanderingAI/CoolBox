#include "extended_kalman_filter.h"

ExtendedKalmanFilter::ExtendedKalmanFilter(
    const Eigen::VectorXd& x0, const Eigen::MatrixXd& P0,
    const Eigen::MatrixXd& Q, const Eigen::MatrixXd& R)
    : x_(x0), P_(P0), Q_(Q), R_(R) {}

void ExtendedKalmanFilter::setProcessModel(
    const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& f,
    const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& F) {
    f_ = f;
    F_ = F;
}

void ExtendedKalmanFilter::setMeasurementModel(
    const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& h,
    const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& H) {
    h_ = h;
    H_ = H;
}

void ExtendedKalmanFilter::predict() {
    Eigen::MatrixXd F_mat = F_(x_);
    x_ = f_(x_);
    P_ = F_mat * P_ * F_mat.transpose() + Q_;
}

void ExtendedKalmanFilter::update(const Eigen::VectorXd& z) {
    Eigen::MatrixXd H_mat = H_(x_);
    Eigen::VectorXd y = z - h_(x_);
    Eigen::MatrixXd S = H_mat * P_ * H_mat.transpose() + R_;
    Eigen::MatrixXd K = P_ * H_mat.transpose() * S.inverse();
    x_ = x_ + K * y;
    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(x_.size(), x_.size());
    P_ = (I - K * H_mat) * P_;
}

const Eigen::VectorXd& ExtendedKalmanFilter::state() const { return x_; }
const Eigen::MatrixXd& ExtendedKalmanFilter::covariance() const { return P_; }
