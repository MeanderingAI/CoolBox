#ifndef BASE_FILTER_H
#define BASE_FILTER_H

#include <Eigen/Dense>

/**
 * @brief Abstract base class for filter implementations.
 *
 * This class defines the interface for filter objects, providing
 * methods for prediction and update steps. Derived classes must
 * implement the predict and update methods to define specific
 * filtering behavior.
 */
class BaseFilter {
public:
    virtual ~BaseFilter() = default;
    /**
     * @brief Predicts the next state.
     */
    virtual void predict() = 0;
    /**
     * @brief Updates the state with a new measurement.
     * @param z Measurement vector.
     */
    virtual void update(const Eigen::VectorXd& z) = 0;
};

#endif // BASE_FILTER_H