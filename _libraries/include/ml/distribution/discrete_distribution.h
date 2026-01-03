#ifndef DISCRETE_DISTRIBUTION_H
#define DISCRETE_DISTRIBUTION_H

#include "ml/distribution/distribution.h"
#include <random>


/**
 * @class DiscreteDistribution
 * @brief An abstract base class for discrete probability distributions.
 */
class DiscreteDistribution : public Distribution {
public:
    virtual ~DiscreteDistribution() = default;

    // Override the base sample function to return a double,
    // as required by the base class. The concrete discrete distributions
    // will need to implement this as well as a discrete sample function.
    double sample() override;

    // Override the base sample function to return an integer type
    virtual int sample_discrete() = 0;
};

#endif