#ifndef BERNOULLI_DISTRIBUTION_H
#define BERNOULLI_DISTRIBUTION_H

#include <discrete_distribution.h>
#include <random>

/**
 * @class BernoulliDistribution
 * @brief A concrete class for the Bernoulli distribution.
 *
 * This distribution uses the canonical logit link function.
 */
class BernoulliDistribution : public DiscreteDistribution {
public:
    BernoulliDistribution(double p);
    double pdf(double x) const override;
    double log_pdf(double x) const override;
    double cdf(double x) const override;
    double log_cdf(double x) const override;
    int sample_discrete() override;

    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;

private:
    std::bernoulli_distribution dist_;
    std::mt19937 gen_;
};

#endif