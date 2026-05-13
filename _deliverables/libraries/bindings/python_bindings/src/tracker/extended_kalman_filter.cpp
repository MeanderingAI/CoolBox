
#include <functional>
#include <Eigen/Dense>
#include <extended_kalman_filter.h>

ExtendedKalmanFilter::ExtendedKalmanFilter(
    const mytrix::Vector& x0,
    const mytrix::Matrix& P0,
    const mytrix::Matrix& Q,
    const mytrix::Matrix& R
)
    : x_(x0), P_(P0), Q_(Q), R_(R)
{}

void ExtendedKalmanFilter::setProcessModel(
    const std::function<mytrix::Vector(const mytrix::Vector&)>& f,
    const std::function<mytrix::Matrix(const mytrix::Vector&)>& F
) {
    f_ = f;
    F_ = F;
}

void ExtendedKalmanFilter::setMeasurementModel(
    const std::function<mytrix::Vector(const mytrix::Vector&)>& h,
    const std::function<mytrix::Matrix(const mytrix::Vector&)>& H
) {
    h_ = h;
    H_ = H;
}

void ExtendedKalmanFilter::predict() {
    if (!f_ || !F_) return;
    x_ = f_(x_);
    mytrix::Matrix Fk = F_(x_);
    P_ = mytrix::Matrix(Fk.data * P_.data * Fk.data.transpose() + Q_.data);
}

void ExtendedKalmanFilter::update(const mytrix::Vector& z) {
    if (!h_ || !H_) return;
    mytrix::Vector y(z.data - h_(x_).data);
    mytrix::Matrix Hk = H_(x_);
    mytrix::Matrix S(Hk.data * P_.data * Hk.data.transpose() + R_.data);
    mytrix::Matrix K(P_.data * Hk.data.transpose() * S.data.inverse());
    x_ = mytrix::Vector(x_.data + K.data * y.data);
    P_ = mytrix::Matrix((Eigen::MatrixXd::Identity(x_.data.size(), x_.data.size()) - K.data * Hk.data) * P_.data);
}

const mytrix::Vector& ExtendedKalmanFilter::state() const {
    return x_;
}

const mytrix::Matrix& ExtendedKalmanFilter::covariance() const {
    return P_;
}
