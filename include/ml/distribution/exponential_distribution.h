#ifndef EXPONENTIAL_DISTRIBUTION_H
#define EXPONENTIAL_DISTRIBUTION_H

#include "ml/distribution/distribution.h"
#include <random>

/**
 * @class ExponentialDistribution
 * @brief A concrete class for the Exponential distribution.
 *
 * Continuous distribution defined by its rate parameter (lambda).
 * 
 * This distribution uses the canonical log link function.
 */
class ExponentialDistribution : public Distribution {
public:
    ExponentialDistribution(double rate);
    double pdf(double x) const override;
    double log_pdf(double x) const override;
    double cdf(double x) const override;
    double log_cdf(double x) const override;
    double sample() override;

    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;

private:
    std::exponential_distribution<double> dist_;
    std::mt19937 gen_;
    double rate_;
};

#endif // EXPONENTIAL_DISTRIBUTION_H