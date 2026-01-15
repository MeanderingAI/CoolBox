/**
 * @file unscented_kalman_filter.h
 * @brief Definition of the UnscentedKalmanFilter class for nonlinear state estimation.
 *
 * This class implements the Unscented Kalman Filter (UKF) algorithm for estimating the state
 * of a nonlinear dynamic system. The UKF uses a deterministic sampling technique to capture
 * the mean and covariance estimates with a minimal set of carefully chosen sample points.
 *
 * @class UnscentedKalmanFilter
 *
 * @brief Public Methods:
 *   - UnscentedKalmanFilter(int state_dim, int meas_dim):
 *       Constructor specifying the dimensions of the state and measurement vectors.
 *   - void initialize(const Vector& x0, const Matrix& P0):
 *       Initializes the filter with an initial state and covariance.
 *   - void predict(const std::function<Vector(const Vector&)>& f, const Matrix& Q):
 *       Performs the prediction step using the process model and process noise covariance.
 *   - void update(const std::function<Vector(const Vector&)>& h, const Vector& z, const Matrix& R):
 *       Performs the update step using the measurement model, measurement, and measurement noise covariance.
 *   - const Vector& getState() const:
 *       Returns the current state estimate.
 *   - const Matrix& getCovariance() const:
 *       Returns the current state covariance.
 *
 * @brief Private Methods:
 *   - void generateSigmaPoints():
 *       Generates sigma points based on the current state and covariance.
 *   - void computeWeights():
 *       Computes the weights for mean and covariance calculations.
 *
 * @brief Member Variables:
 *   - n_x_: State dimension.
 *   - n_z_: Measurement dimension.
 *   - lambda_: Scaling parameter for sigma point generation.
 *   - x_: Current state estimate.
 *   - P_: Current state covariance.
 *   - sigma_points_: Container for generated sigma points.
 *   - weights_mean_: Weights for mean calculation.
 *   - weights_cov_: Weights for covariance calculation.
 *   - alpha_, beta_, kappa_: UKF tuning parameters.
 *
 * @note
 *   - Requires Eigen library for matrix and vector operations.
 *   - Designed for extensibility and integration with arbitrary nonlinear models.
 */
#ifndef UNSCENTED_KALMAN_FILTER_H
#define UNSCENTED_KALMAN_FILTER_H

#include <Eigen/Dense>
#include <vector>
#include "ML/tracker/base_kalman_filter.h"

class UnscentedKalmanFilter : public BaseKalmanFilter {
public:
    using Vector = Eigen::VectorXd;
    using Matrix = Eigen::MatrixXd;

    UnscentedKalmanFilter(int state_dim, int meas_dim);

    void initialize(const Vector& x0, const Matrix& P0);

    // Setters for models and noise covariances
    void setProcessModel(const std::function<Vector(const Vector&)>& f, const Matrix& Q);
    void setMeasurementModel(const std::function<Vector(const Vector&)>& h, const Matrix& R);

    // Unified interface overrides
    void predict() override;
    void update(const Eigen::VectorXd& z) override;

    const Vector& state() const override;
    const Matrix& covariance() const override;

private:
    void generateSigmaPoints();
    void computeWeights();

    int n_x_; // State dimension
    int n_z_; // Measurement dimension
    double lambda_;
    Vector x_; // State estimate
    Matrix P_; // State covariance

    std::vector<Vector> sigma_points_;
    Vector weights_mean_;
    Vector weights_cov_;

    // UKF parameters
    double alpha_ = 1e-3;
    double beta_ = 2.0;
    double kappa_ = 0.0;
    
    // Nonlinear models and noise
    std::function<Vector(const Vector&)> f_;
    std::function<Vector(const Vector&)> h_;
    Matrix Q_;
    Matrix R_;
    Vector z_; // Last measurement
};

#endif // UNSCENTED_KALMAN_FILTER_H