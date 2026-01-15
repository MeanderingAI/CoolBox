#ifndef POISSON_DISTRIBUTION_H
#define POISSON_DISTRIBUTION_H

#include "ML/distribution/discrete_distribution.h"
#include <random>


/**
 * @class PoissonDistribution
 * @brief A concrete class for the Poisson distribution.
 *
 * This distribution uses the canonical log link function.
 */
class PoissonDistribution : public DiscreteDistribution {
public:
    PoissonDistribution(double lambda);
    double pdf(double x) const override;
    double log_pdf(double x) const override;
    double cdf(double x) const override;
    double log_cdf(double x) const override;
    int sample_discrete() override;

    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;

private:
    std::poisson_distribution<int> dist_;
    std::mt19937 gen_;
    double lambda_;
};

#endif 