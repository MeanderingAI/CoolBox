#ifndef BASE_KALMAN_FILTER_H
#define BASE_KALMAN_FILTER_H

#include "mytrix_eigen_compat.hpp"
// All matrix/vector types now use mytrix::Matrix, mytrix::Vector, etc.
#include "base_filter.h"

/**
 * @brief Abstract base class for Kalman filter variants.
 */
class BaseKalmanFilter : public BaseFilter {
public:
    virtual ~BaseKalmanFilter() = default;

    /**
     * @brief Returns the current state estimate.
     */
    virtual const mytrix::Vector& state() const = 0;

    /**
     * @brief Returns the current state covariance.
     */
    virtual const mytrix::Matrix& covariance() const = 0;
};

#endif // BASE_KALMAN_FILTER_H