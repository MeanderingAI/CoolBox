#ifndef GAMMA_DISTRIBUTION_H
#define GAMMA_DISTRIBUTION_H

#include "ml/distribution/distribution.h"
#include <random>

/**
 * @class GammaDistribution
 * @brief A concrete class for the Gamma distribution.
 *
 * This distribution uses the canonical inverse link function.
 */
class GammaDistribution : public Distribution {
public:
    GammaDistribution(double shape, double rate);
    double pdf(double x) const override;
    double log_pdf(double x) const override;
    double cdf(double x) const override;
    double log_cdf(double x) const override;
    double sample() override;

    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;

private:
    std::gamma_distribution<double> dist_;
    std::mt19937 gen_;
    double shape_;
    double rate_;
};
#endif // GAMMA_DISTRIBUTION_H