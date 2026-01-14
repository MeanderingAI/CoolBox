#ifndef BINOMIAL_DISTRIBUTION_H
#define BINOMIAL_DISTRIBUTION_H

#include "ML/distribution/discrete_distribution.h"
#include <random>

/**
 * @class BinomialDistribution
 * @brief A concrete class for the Binomial distribution.
 *
 * This distribution uses the canonical logit link function.
 */
class BinomialDistribution : public DiscreteDistribution {
public:
    BinomialDistribution(int t, double p);
    double pdf(double x) const override;
    double log_pdf(double x) const override;
    double cdf(double x) const override;
    double log_cdf(double x) const override;
    int sample_discrete() override;

    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;

private:
    std::binomial_distribution<int> dist_;
    std::mt19937 gen_;
};

#endif