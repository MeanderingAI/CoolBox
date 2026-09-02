#include <discrete_distribution.h>

// Override the base sample function to return a double,
// as required by the base class. This function simply calls the
// pure virtual sample_discrete() and casts the result.
double DiscreteDistribution::sample() {
    return static_cast<double>(sample_discrete());
}
