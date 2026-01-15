#ifndef NORMAL_DISTRIBUTION_H
#define NORMAL_DISTRIBUTION_H

#include <distribution.h>
#include <random>

/**
 * @class NormalDistribution
 * @brief A concrete class for the Normal (Gaussian) distribution.
 * 
 * Continuous distribution defined by its mean and standard deviation.
 * 
 * This distribution uses the canonical identity link function.
 */
class NormalDistribution : public Distribution {
public:
    NormalDistribution(double mean, double stddev);
    double pdf(double x) const override;
    double log_pdf(double x) const override;
    double cdf(double x) const override;
    double log_cdf(double x) const override;
    double sample() override;
    
    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;

private:
    std::normal_distribution<double> dist_;
    std::mt19937 gen_;
    double mean_;
    double stddev_;
};

#endif // NORMAL_DISTRIBUTION_H