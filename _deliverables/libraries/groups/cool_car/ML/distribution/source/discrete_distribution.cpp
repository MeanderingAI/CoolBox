#include "discrete_distribution.h"

double DiscreteDistribution::sample() {
    return static_cast<double>(sample_discrete());
}
