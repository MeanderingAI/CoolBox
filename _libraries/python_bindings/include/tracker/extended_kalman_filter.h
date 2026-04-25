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
 *     const mytrix::Vector& x0,   ///< Initial state vector
 *     const mytrix::Matrix& P0,   ///< Initial state covariance matrix
 *     const mytrix::Matrix& Q,    ///< Process noise covariance matrix
 *     const mytrix::Matrix& R     ///< Measurement noise covariance matrix
 * )
 *
 * @method void predict(
 *     const std::function<mytrix::Vector(const mytrix::Vector&)>& f, ///< Nonlinear process model
 *     const std::function<mytrix::Matrix(const mytrix::Vector&)>& F  ///< Jacobian of process model
 * )
 * @brief Predicts the next state and covariance using the process model.
 *
 * @method void update(
 *     const mytrix::Vector& z,    ///< Measurement vector
 *     const std::function<mytrix::Vector(const mytrix::Vector&)>& h, ///< Nonlinear measurement model
 *     const std::function<mytrix::Matrix(const mytrix::Vector&)>& H  ///< Jacobian of measurement model
 * )
 * @brief Updates the state and covariance using the measurement.
 *
 * @method const mytrix::Vector& state() const
 * @brief Returns the current state estimate.
 *
 * @method const mytrix::Matrix& covariance() const
 * @brief Returns the current state covariance.
 *
 * @private
 * mytrix::Vector x_; ///< Current state estimate
 * mytrix::Matrix P_; ///< Current state covariance
 * mytrix::Matrix Q_; ///< Process noise covariance
 * mytrix::Matrix R_; ///< Measurement noise covariance
 */
#ifndef EXTENDED_KALMAN_FILTER_H
#define EXTENDED_KALMAN_FILTER_H

#include "mytrix_eigen_compat.hpp"
#include "base_kalman_filter.h"

class ExtendedKalmanFilter : public BaseKalmanFilter {
public:
    ExtendedKalmanFilter(
        const mytrix::Vector& x0,
        const mytrix::Matrix& P0,
        const mytrix::Matrix& Q,
        const mytrix::Matrix& R
    );
    // Add setters for models
    void setProcessModel(const std::function<mytrix::Vector(const mytrix::Vector&)>& f,
                         const std::function<mytrix::Matrix(const mytrix::Vector&)>& F);

    void setMeasurementModel(const std::function<mytrix::Vector(const mytrix::Vector&)>& h,
                             const std::function<mytrix::Matrix(const mytrix::Vector&)>& H);


    void predict() override;
    void update(const mytrix::Vector& z) override;

    const mytrix::Vector& state() const override;
    const mytrix::Matrix& covariance() const override;

private:
    std::function<mytrix::Vector(const mytrix::Vector&)> f_;
    std::function<mytrix::Matrix(const mytrix::Vector&)> F_;
    std::function<mytrix::Vector(const mytrix::Vector&)> h_;
    std::function<mytrix::Matrix(const mytrix::Vector&)> H_;
    mytrix::Vector x_;
    mytrix::Matrix P_;
    mytrix::Matrix Q_;
    mytrix::Matrix R_;
};

#endif // EXTENDED_KALMAN_FILTER_H