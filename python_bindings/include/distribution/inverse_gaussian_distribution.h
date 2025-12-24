#ifndef INVERSE_GAUSSIAN_DISTRIBUTION_H
#define INVERSE_GAUSSIAN_DISTRIBUTION_H

#include <distribution.h>
#include <random>

/**
 * @class InverseGaussianDistribution
 * @brief A concrete class for the Inverse Gaussian distribution.
 *
 * This distribution uses the canonical inverse squared link function.
 */
class InverseGaussianDistribution : public Distribution {
public:
    InverseGaussianDistribution(double mean, double shape);
    double pdf(double x) const override;
    double log_pdf(double x) const override;
    double cdf(double x) const override;
    double log_cdf(double x) const override;
    double sample() override;

    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;

private:
    std::mt19937 gen_;
    double mean_;
    double shape_;
};

#endif