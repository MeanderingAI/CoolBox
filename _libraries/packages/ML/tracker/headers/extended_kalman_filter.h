/**
 * @file extended_kalman_filter.h
 * @brief Defines the ExtendedKalmanFilter class for nonlinear state estimation.
 *
 * This class implements an Extended Kalman Filter (EKF) for estimating the state of a nonlinear dynamic system.
 * The EKF uses nonlinear process and measurement models, linearized via user-provided Jacobians.
 *
 * @class ExtendedKalmanFilter
 * @brief Extended Kalman Filter for nonlinear systems.
 *
 * @section Usage
 * - Construct the filter with initial state, covariance, process noise, and measurement noise.
 * - Call predict() with the nonlinear process model and its Jacobian.
 * - Call update() with the measurement, nonlinear measurement model, and its Jacobian.
 *
 * Constructor:
 *   ExtendedKalmanFilter(const Eigen::VectorXd& x0, const Eigen::MatrixXd& P0, const Eigen::MatrixXd& Q, const Eigen::MatrixXd& R)
 *     - x0: Initial state vector
 *     - P0: Initial state covariance matrix
 *     - Q: Process noise covariance matrix
 *     - R: Measurement noise covariance matrix
 *
 * Methods:
 *   void predict(const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& f, const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& F)
 *     - Predicts the next state and covariance using the process model.
 *   void update(const Eigen::VectorXd& z, const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& h, const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& H)
 *     - Updates the state and covariance using the measurement.
 *   const Eigen::VectorXd& state() const
 *     - Returns the current state estimate.
 *   const Eigen::MatrixXd& covariance() const
 *     - Returns the current state covariance.
 *
 * @private
 * Eigen::VectorXd x_; ///< Current state estimate
 * Eigen::MatrixXd P_; ///< Current state covariance
 * Eigen::MatrixXd Q_; ///< Process noise covariance
 * Eigen::MatrixXd R_; ///< Measurement noise covariance
 */
 #ifndef EXTENDED_KALMAN_FILTER_H
 #define EXTENDED_KALMAN_FILTER_H

#include "../../../DATASTRUCTURE/matrix/headers/matrix_dense.h"
 #include "base_kalman_filter.h"
 #include <vector>
 #include <functional>

class ExtendedKalmanFilter : public BaseKalmanFilter {
public:
    using Vector = mytrix::Vector;
    using Matrix = mytrix::Matrix;

    ExtendedKalmanFilter(
        const Vector& x0,
        const Matrix& P0,
        const Matrix& Q,
        const Matrix& R
    );
    void setProcessModel(const std::function<Vector(const Vector&)>& f,
                         const std::function<Matrix(const Vector&)>& F);

    void setMeasurementModel(const std::function<Vector(const Vector&)>& h,
                             const std::function<Matrix(const Vector&)>& H);
    void predict() override;
    void update(const Vector& z) override;
    const Vector& state() const override;
    const Matrix& covariance() const override;

private:
    Vector x_;
    Matrix P_;
    Matrix Q_;
    Matrix R_;
    std::function<Vector(const Vector&)> f_;
    std::function<Matrix(const Vector&)> F_;
    std::function<Vector(const Vector&)> h_;
    std::function<Matrix(const Vector&)> H_;
};

 #endif // EXTENDED_KALMAN_FILTER_H