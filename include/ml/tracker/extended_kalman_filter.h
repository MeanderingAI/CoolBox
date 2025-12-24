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
 * @constructor
 * ExtendedKalmanFilter(
 *     const Eigen::VectorXd& x0,   ///< Initial state vector
 *     const Eigen::MatrixXd& P0,   ///< Initial state covariance matrix
 *     const Eigen::MatrixXd& Q,    ///< Process noise covariance matrix
 *     const Eigen::MatrixXd& R     ///< Measurement noise covariance matrix
 * )
 *
 * @method void predict(
 *     const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& f, ///< Nonlinear process model
 *     const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& F  ///< Jacobian of process model
 * )
 * @brief Predicts the next state and covariance using the process model.
 *
 * @method void update(
 *     const Eigen::VectorXd& z,    ///< Measurement vector
 *     const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& h, ///< Nonlinear measurement model
 *     const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& H  ///< Jacobian of measurement model
 * )
 * @brief Updates the state and covariance using the measurement.
 *
 * @method const Eigen::VectorXd& state() const
 * @brief Returns the current state estimate.
 *
 * @method const Eigen::MatrixXd& covariance() const
 * @brief Returns the current state covariance.
 *
 * @private
 * Eigen::VectorXd x_; ///< Current state estimate
 * Eigen::MatrixXd P_; ///< Current state covariance
 * Eigen::MatrixXd Q_; ///< Process noise covariance
 * Eigen::MatrixXd R_; ///< Measurement noise covariance
 */
#ifndef EXTENDED_KALMAN_FILTER_H
#define EXTENDED_KALMAN_FILTER_H

#include <Eigen/Dense>
#include "ml/tracker/base_kalman_filter.h"

class ExtendedKalmanFilter : public BaseKalmanFilter {
public:
    ExtendedKalmanFilter(
        const Eigen::VectorXd& x0,
        const Eigen::MatrixXd& P0,
        const Eigen::MatrixXd& Q,
        const Eigen::MatrixXd& R
    );
    // Add setters for models
    void setProcessModel(const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& f,
                         const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& F);

    void setMeasurementModel(const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& h,
                             const std::function<Eigen::MatrixXd(const Eigen::VectorXd&)>& H);


    void predict() override;
    void update(const Eigen::VectorXd& z) override;

    const Eigen::VectorXd& state() const override;
    const Eigen::MatrixXd& covariance() const override;

private:
    std::function<Eigen::VectorXd(const Eigen::VectorXd&)> f_;
    std::function<Eigen::MatrixXd(const Eigen::VectorXd&)> F_;
    std::function<Eigen::VectorXd(const Eigen::VectorXd&)> h_;
    std::function<Eigen::MatrixXd(const Eigen::VectorXd&)> H_;
    Eigen::VectorXd x_;
    Eigen::MatrixXd P_;
    Eigen::MatrixXd Q_;
    Eigen::MatrixXd R_;
};

#endif // EXTENDED_KALMAN_FILTER_H