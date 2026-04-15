#include "unscented_kalman_filter.h"
#include <cmath>

UnscentedKalmanFilter::UnscentedKalmanFilter(int state_dim, int meas_dim)
    : n_x_(state_dim), n_z_(meas_dim) {
    x_ = Eigen::VectorXd::Zero(n_x_);
    P_ = Eigen::MatrixXd::Identity(n_x_, n_x_);
    Q_ = Eigen::MatrixXd::Identity(n_x_, n_x_) * 0.01;
    R_ = Eigen::MatrixXd::Identity(n_z_, n_z_) * 0.1;
    computeWeights();
}

void UnscentedKalmanFilter::initialize(const Vector& x0, const Matrix& P0) {
    x_ = x0;
    P_ = P0;
}

void UnscentedKalmanFilter::setProcessModel(
    const std::function<Vector(const Vector&)>& f, const Matrix& Q) {
    f_ = f;
    Q_ = Q;
}

void UnscentedKalmanFilter::setMeasurementModel(
    const std::function<Vector(const Vector&)>& h, const Matrix& R) {
    h_ = h;
    R_ = R;
}

void UnscentedKalmanFilter::computeWeights() {
    lambda_ = alpha_ * alpha_ * (n_x_ + kappa_) - n_x_;
    int n_sigma = 2 * n_x_ + 1;
    weights_mean_ = Vector::Constant(n_sigma, 1.0 / (2.0 * (n_x_ + lambda_)));
    weights_cov_ = Vector::Constant(n_sigma, 1.0 / (2.0 * (n_x_ + lambda_)));
    weights_mean_(0) = lambda_ / (n_x_ + lambda_);
    weights_cov_(0) = lambda_ / (n_x_ + lambda_) + (1.0 - alpha_ * alpha_ + beta_);
}

void UnscentedKalmanFilter::generateSigmaPoints() {
    int n_sigma = 2 * n_x_ + 1;
    sigma_points_.resize(n_sigma);
    
    Matrix L = ((n_x_ + lambda_) * P_).llt().matrixL();
    
    sigma_points_[0] = x_;
    for (int i = 0; i < n_x_; ++i) {
        sigma_points_[i + 1] = x_ + L.col(i);
        sigma_points_[n_x_ + i + 1] = x_ - L.col(i);
    }
}

void UnscentedKalmanFilter::predict() {
    generateSigmaPoints();
    int n_sigma = 2 * n_x_ + 1;
    
    // Transform sigma points through process model
    std::vector<Vector> transformed(n_sigma);
    for (int i = 0; i < n_sigma; ++i) {
        transformed[i] = f_(sigma_points_[i]);
    }
    
    // Compute predicted mean
    x_ = Vector::Zero(n_x_);
    for (int i = 0; i < n_sigma; ++i) {
        x_ += weights_mean_(i) * transformed[i];
    }
    
    // Compute predicted covariance
    P_ = Q_;
    for (int i = 0; i < n_sigma; ++i) {
        Vector diff = transformed[i] - x_;
        P_ += weights_cov_(i) * diff * diff.transpose();
    }
    
    sigma_points_ = transformed;
}

void UnscentedKalmanFilter::update(const Eigen::VectorXd& z) {
    z_ = z;
    int n_sigma = 2 * n_x_ + 1;
    
    // Transform sigma points through measurement model
    std::vector<Vector> z_sigma(n_sigma);
    for (int i = 0; i < n_sigma; ++i) {
        z_sigma[i] = h_(sigma_points_[i]);
    }
    
    // Predicted measurement mean
    Vector z_pred = Vector::Zero(n_z_);
    for (int i = 0; i < n_sigma; ++i) {
        z_pred += weights_mean_(i) * z_sigma[i];
    }
    
    // Innovation covariance
    Matrix S = R_;
    for (int i = 0; i < n_sigma; ++i) {
        Vector diff = z_sigma[i] - z_pred;
        S += weights_cov_(i) * diff * diff.transpose();
    }
    
    // Cross-covariance
    Matrix Pxz = Matrix::Zero(n_x_, n_z_);
    for (int i = 0; i < n_sigma; ++i) {
        Vector x_diff = sigma_points_[i] - x_;
        Vector z_diff = z_sigma[i] - z_pred;
        Pxz += weights_cov_(i) * x_diff * z_diff.transpose();
    }
    
    // Kalman gain
    Matrix K = Pxz * S.inverse();
    x_ = x_ + K * (z - z_pred);
    P_ = P_ - K * S * K.transpose();
}

const UnscentedKalmanFilter::Vector& UnscentedKalmanFilter::state() const { return x_; }
const UnscentedKalmanFilter::Matrix& UnscentedKalmanFilter::covariance() const { return P_; }
