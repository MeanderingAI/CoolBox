#ifndef CATEGORICAL_DISTRIBUTION_H
#define CATEGORICAL_DISTRIBUTION_H

#include "ml/distribution/discrete_distribution.h"
#include <random>

/**
 * @class CategoricalDistribution
 * @brief A concrete class for the Categorical distribution.
 *
 * This distribution does not have a standard single-parameter GLM form.
 * The link functions are not implemented as they do not apply.
 */
class CategoricalDistribution : public DiscreteDistribution {
public:
    CategoricalDistribution(const std::vector<double>& weights);
    double pdf(double x) const override;
    double log_pdf(double x) const override;
    double cdf(double x) const override;
    double log_cdf(double x) const override;
    int sample_discrete() override;
    
    // GLM functions are not applicable in a single-parameter sense.
    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;

private:
    std::discrete_distribution<int> dist_;
    std::mt19937 gen_;
    std::vector<double> weights_;
};

#endif