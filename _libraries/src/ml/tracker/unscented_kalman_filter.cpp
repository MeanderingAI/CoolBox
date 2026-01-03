
#include <Eigen/Dense>
#include <vector>
#include <cmath>
#include "ml/tracker/unscented_kalman_filter.h"

UnscentedKalmanFilter::UnscentedKalmanFilter(int state_dim, int meas_dim)
    : n_x_(state_dim), n_z_(meas_dim)
{
    lambda_ = alpha_ * alpha_ * (n_x_ + kappa_) - n_x_;
    x_ = Vector::Zero(n_x_);
    P_ = Matrix::Identity(n_x_, n_x_);
    weights_mean_ = Vector::Zero(2 * n_x_ + 1);
    weights_cov_ = Vector::Zero(2 * n_x_ + 1);
    computeWeights();
}

void UnscentedKalmanFilter::initialize(const Vector& x0, const Matrix& P0)
{
    x_ = x0;
    P_ = P0;
}

void UnscentedKalmanFilter::setProcessModel(const std::function<Vector(const Vector&)>& f, const Matrix& Q)
{
    f_ = f;
    Q_ = Q;
}

void UnscentedKalmanFilter::setMeasurementModel(const std::function<Vector(const Vector&)>& h, const Matrix& R)
{
    h_ = h;
    R_ = R;
}

void UnscentedKalmanFilter::computeWeights()
{
    double denom = n_x_ + lambda_;
    weights_mean_(0) = lambda_ / denom;
    weights_cov_(0) = lambda_ / denom + (1 - alpha_ * alpha_ + beta_);
    for (int i = 1; i < 2 * n_x_ + 1; ++i) {
        weights_mean_(i) = 1.0 / (2.0 * denom);
        weights_cov_(i) = 1.0 / (2.0 * denom);
    }
}

void UnscentedKalmanFilter::generateSigmaPoints()
{
    sigma_points_.clear();
    Matrix A = P_.llt().matrixL();
    sigma_points_.push_back(x_);
    double scaling = std::sqrt(n_x_ + lambda_);
    for (int i = 0; i < n_x_; ++i) {
        sigma_points_.push_back(x_ + scaling * A.col(i));
        sigma_points_.push_back(x_ - scaling * A.col(i));
    }
}

void UnscentedKalmanFilter::predict()
{
    if (!f_) return; // Optionally throw or assert
    generateSigmaPoints();
    std::vector<Vector> propagated_sigma;
    for (const auto& pt : sigma_points_) {
        propagated_sigma.push_back(f_(pt));
    }

    // Predict mean
    x_ = Vector::Zero(n_x_);
    for (size_t i = 0; i < propagated_sigma.size(); ++i) {
        x_ += weights_mean_(i) * propagated_sigma[i];
    }

    // Predict covariance
    P_ = Matrix::Zero(n_x_, n_x_);
    for (size_t i = 0; i < propagated_sigma.size(); ++i) {
        Vector dx = propagated_sigma[i] - x_;
        P_ += weights_cov_(i) * dx * dx.transpose();
    }
    P_ += Q_;
}

void UnscentedKalmanFilter::update(const Eigen::VectorXd& z)
{
    if (!h_) return; // Optionally throw or assert
    z_ = z; // Store last measurement if needed

    // Transform sigma points through measurement function
    std::vector<Vector> meas_sigma;
    for (const auto& pt : sigma_points_) {
        meas_sigma.push_back(h_(pt));
    }

    // Predicted measurement mean
    Vector z_pred = Vector::Zero(n_z_);
    for (size_t i = 0; i < meas_sigma.size(); ++i) {
        z_pred += weights_mean_(i) * meas_sigma[i];
    }

    // Innovation covariance
    Matrix S = Matrix::Zero(n_z_, n_z_);
    for (size_t i = 0; i < meas_sigma.size(); ++i) {
        Vector dz = meas_sigma[i] - z_pred;
        S += weights_cov_(i) * dz * dz.transpose();
    }
    S += R_;

    // Cross covariance
    Matrix Tc = Matrix::Zero(n_x_, n_z_);
    for (size_t i = 0; i < meas_sigma.size(); ++i) {
        Vector dx = sigma_points_[i] - x_;
        Vector dz = meas_sigma[i] - z_pred;
        Tc += weights_cov_(i) * dx * dz.transpose();
    }

    // Kalman gain
    Matrix K = Tc * S.inverse();

    // Update state and covariance
    x_ = x_ + K * (z - z_pred);
    P_ = P_ - K * S * K.transpose();
}

const UnscentedKalmanFilter::Vector& UnscentedKalmanFilter::state() const
{
    return x_;
}

const UnscentedKalmanFilter::Matrix& UnscentedKalmanFilter::covariance() const
{
    return P_;
}