#ifndef MULTINOMIAL_DISTRIBUTION_H
#define MULTINOMIAL_DISTRIBUTION_H

#include <discrete_distribution.h>
#include <random>

/**
 * @class MultinomialDistribution
 * @brief A concrete class for the Multinomial distribution.
 *
 * Similar to the Categorical distribution, the link functions are not applicable.
 */
class MultinomialDistribution : public DiscreteDistribution {
public:
    MultinomialDistribution(int trials, const std::vector<double>& probabilities);
    
    // For this distribution, we overload pdf to take a vector of counts.
    double pdf(const std::vector<int>& counts) const;

    // The base class methods are not meaningful for this distribution.
    double pdf(double x) const override { return 0.0; }
    double log_pdf(double x) const override { return -std::numeric_limits<double>::infinity(); }
    double cdf(double x) const override { return 0.0; }
    double log_cdf(double x) const override { return -std::numeric_limits<double>::infinity(); }
    double sample() override; // Samples a single outcome
    std::vector<int> sample_multinomial(); // Samples a vector of outcomes

    // GLM functions are not applicable.
    std::string link_name() const override;
    double link_function(double mu) const override;
    double mean_function(double eta) const override;
    
private:
    int trials_;
    std::vector<double> probabilities_;
    std::discrete_distribution<int> dist_;
    std::mt19937 gen_;
};

#endif