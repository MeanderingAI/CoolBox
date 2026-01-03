#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include <Eigen/Dense>
#include "ml/tracker/base_kalman_filter.h"

class KalmanFilter : public BaseKalmanFilter {
public:/**
     * @brief Constructor for the Kalman Filter.
     * @param dt Time step (e.g., in seconds).
     * @param A State transition matrix.
     * @param C Observation matrix.
     * @param Q Process noise covariance matrix.
     * @param R Measurement noise covariance matrix.
     * @param P Initial estimate error covariance matrix.
     */
    KalmanFilter(double dt,
                 const Eigen::MatrixXd& A,
                 const Eigen::MatrixXd& C,
                 const Eigen::MatrixXd& Q,
                 const Eigen::MatrixXd& R,
                 const Eigen::MatrixXd& P);
                 
    /**
     * @brief Initializes the filter with an initial state and time step.
     * @param x0 Initial state vector.
     */
    void init(const Eigen::VectorXd& x0);

    /**
     * @brief Predicts the next state.
     */
    void predict() override;

    /**
     * @brief Updates the state with a new measurement.
     * @param y Measurement vector.
     */
    void update(const Eigen::VectorXd& y) override;

    /**
     * @brief Returns the current state estimate.
     */
    const Eigen::VectorXd& state() const override;


    /**
     * @brief Returns the current state estimate.
     */
    const Eigen::MatrixXd& covariance() const override;
private:
    // Time step
    double dt;

    // State vectors
    Eigen::VectorXd x; // state vector
    Eigen::MatrixXd P; // estimate error covariance

    // System matrices
    Eigen::MatrixXd A; // state transition matrix
    Eigen::MatrixXd C; // observation matrix
    Eigen::MatrixXd Q; // process noise covariance
    Eigen::MatrixXd R; // measurement noise covariance
};

#endif // KALMAN_FILTER_H