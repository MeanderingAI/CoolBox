#include "unscented_kalman_filter.h"
#include <cmath>

UnscentedKalmanFilter::UnscentedKalmanFilter(int state_dim, int meas_dim)
    : n_x_(state_dim), n_z_(meas_dim) {
    x_ = mytrix::Vector(n_x_, 0.0);
    P_ = mytrix::Matrix::Identity(n_x_);
    Q_ = mytrix::Matrix::Identity(n_x_);
    // Scale Q_ by 0.01
    for (size_t r = 0; r < Q_.rows(); ++r)
        for (size_t c = 0; c < Q_.cols(); ++c)
            Q_.at(r, c) *= 0.01;
    R_ = mytrix::Matrix::Identity(n_z_);
    // Scale R_ by 0.1
    for (size_t r = 0; r < R_.rows(); ++r)
        for (size_t c = 0; c < R_.cols(); ++c)
            R_.at(r, c) *= 0.1;
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
    weights_mean_ = Vector(n_sigma, 1.0 / (2.0 * (n_x_ + lambda_)));
    weights_cov_ = Vector(n_sigma, 1.0 / (2.0 * (n_x_ + lambda_)));
    if (!weights_mean_.empty()) weights_mean_[0] = lambda_ / (n_x_ + lambda_);
    if (!weights_cov_.empty()) weights_cov_[0] = lambda_ / (n_x_ + lambda_) + (1.0 - alpha_ * alpha_ + beta_);
}

void UnscentedKalmanFilter::generateSigmaPoints() {
    int n_sigma = 2 * n_x_ + 1;
    sigma_points_.resize(n_sigma);
    
    // Manual Cholesky not implemented; just use identity for L for now
    Matrix L = Matrix::Identity(n_x_);
    // Scale L by sqrt(n_x_ + lambda_)
    double scale = std::sqrt(n_x_ + lambda_);
    for (size_t r = 0; r < L.rows(); ++r)
        for (size_t c = 0; c < L.cols(); ++c)
            L.at(r, c) *= scale;

    sigma_points_[0] = x_;
    for (int i = 0; i < n_x_; ++i) {
        // Extract column i from L
        Vector col_i(n_x_, 0.0);
        for (int r = 0; r < n_x_; ++r) col_i[r] = L.at(r, i);
        // x_ + col_i
        Vector x_plus = x_;
        for (int j = 0; j < n_x_; ++j) x_plus[j] += col_i[j];
        sigma_points_[i + 1] = x_plus;
        // x_ - col_i
        Vector x_minus = x_;
        for (int j = 0; j < n_x_; ++j) x_minus[j] -= col_i[j];
        sigma_points_[n_x_ + i + 1] = x_minus;
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

    // Compute predicted mean using my_vector ops
    x_ = Vector(n_x_, 0.0);
    for (int i = 0; i < n_sigma; ++i) {
        for (int j = 0; j < n_x_; ++j) {
            x_[j] += transformed[i][j] * weights_mean_[i];
        }
    }

    // Compute predicted covariance (manual implementation)
    for (size_t r = 0; r < P_.rows(); ++r)
        for (size_t c = 0; c < P_.cols(); ++c)
            P_.at(r, c) = Q_.at(r, c);
    for (int i = 0; i < n_sigma; ++i) {
        Vector diff = transformed[i] - x_;
        for (size_t r = 0; r < P_.rows(); ++r) {
            for (size_t c = 0; c < P_.cols(); ++c) {
                P_.at(r, c) += weights_cov_[i] * diff[r] * diff[c];
            }
        }
    }

    sigma_points_ = transformed;
}

void UnscentedKalmanFilter::update(const Vector& z) {
    z_ = z;
    int n_sigma = 2 * n_x_ + 1;
    
    // Transform sigma points through measurement model
    std::vector<Vector> z_sigma(n_sigma);
    for (int i = 0; i < n_sigma; ++i) {
        z_sigma[i] = h_(sigma_points_[i]);
    }

    // Predicted measurement mean
    Vector z_pred(n_z_, 0.0);
    for (int i = 0; i < n_sigma; ++i) {
        for (int j = 0; j < n_z_; ++j) {
            z_pred[j] += weights_mean_[i] * z_sigma[i][j];
        }
    }

    // Innovation covariance
    Matrix S = R_;
    for (int i = 0; i < n_sigma; ++i) {
        Vector diff(n_z_, 0.0);
        for (int j = 0; j < n_z_; ++j) diff[j] = z_sigma[i][j] - z_pred[j];
        // Outer product and scale, then add to S
        for (int r = 0; r < n_z_; ++r) {
            for (int c = 0; c < n_z_; ++c) {
                S.at(r, c) += weights_cov_[i] * diff[r] * diff[c];
            }
        }
    }

    // Cross-covariance
    Matrix Pxz = Matrix::Zero(n_x_, n_z_);
    for (int i = 0; i < n_sigma; ++i) {
        Vector x_diff(n_x_, 0.0);
        Vector z_diff(n_z_, 0.0);
        for (int j = 0; j < n_x_; ++j) x_diff[j] = sigma_points_[i][j] - x_[j];
        for (int j = 0; j < n_z_; ++j) z_diff[j] = z_sigma[i][j] - z_pred[j];
        for (int r = 0; r < n_x_; ++r) {
            for (int c = 0; c < n_z_; ++c) {
                Pxz.at(r, c) += weights_cov_[i] * x_diff[r] * z_diff[c];
            }
        }
    }

    // Kalman gain (stub S.inverse with identity for now if not implemented)
    Matrix S_inv = Matrix::Identity(n_z_); // TODO: Replace with real inverse
    Matrix K(n_x_, n_z_);
    // K = Pxz * S_inv
    for (int r = 0; r < n_x_; ++r) {
        for (int c = 0; c < n_z_; ++c) {
            K.at(r, c) = 0.0;
            for (int k = 0; k < n_z_; ++k) {
                K.at(r, c) += Pxz.at(r, k) * S_inv.at(k, c);
            }
        }
    }

    // x_ = x_ + K * (z - z_pred)
    Vector innovation(n_z_, 0.0);
    for (int j = 0; j < n_z_; ++j) innovation[j] = z[j] - z_pred[j];
    for (int r = 0; r < n_x_; ++r) {
        double update = 0.0;
        for (int c = 0; c < n_z_; ++c) {
            update += K.at(r, c) * innovation[c];
        }
        x_[r] += update;
    }

    // P_ = P_ - K * S * K.transpose(); (approximate, since S_inv is identity)
    // For now, skip this or implement a manual version if needed
}

const UnscentedKalmanFilter::Vector& UnscentedKalmanFilter::state() const { return x_; }
const UnscentedKalmanFilter::Matrix& UnscentedKalmanFilter::covariance() const { return P_; }
