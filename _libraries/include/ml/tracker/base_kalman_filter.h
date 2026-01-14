#ifndef BASE_KALMAN_FILTER_H
#define BASE_KALMAN_FILTER_H

#include <Eigen/Dense>
#include "ML/tracker/base_filter.h"

/**
 * @brief Abstract base class for Kalman filter variants.
 */
class BaseKalmanFilter : public BaseFilter {
public:
    virtual ~BaseKalmanFilter() = default;

    /**
     * @brief Returns the current state estimate.
     */
    virtual const Eigen::VectorXd& state() const = 0;

    /**
     * @brief Returns the current state covariance.
     */
    virtual const Eigen::MatrixXd& covariance() const = 0;
};

#endif // BASE_KALMAN_FILTER_H