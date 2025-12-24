#ifndef LAPLACE_DISTRIBUTION_H
#define LAPLACE_DISTRIBUTION_H

#include <distribution.h>
#include <random>

/**
 * @class LaplaceDistribution
 * @brief A concrete class for the Laplace distribution.
 *
 * This distribution is not a standard member of the exponential family.
 * An identity link is provided as a simple, common choice.
 */
class LaplaceDistribution : public Distribution {
public:
    LaplaceDistribution(double loc, double scale);
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
    double location_;
    double scale_;
};
#endif // LAPLACE_DISTRIBUTION_H