#include "ML/distribution/categorical_distribution.h"
#include <stdexcept>

// Constructor
CategoricalDistribution::CategoricalDistribution(const std::vector<double>& p)
    : weights_(p) {
    double sum = std::accumulate(weights_.begin(), weights_.end(), 0.0);
    if (std::abs(sum - 1.0) > 1e-9) {
        throw std::invalid_argument("Probabilities must sum to 1.0");
    }
}

// Probability Mass Function
double CategoricalDistribution::pdf(double x) const {
    int index = static_cast<int>(x);
    if (index >= 0 && index < weights_.size()) {
        return weights_[index];
    }
    return 0.0;
}

// Log of the Probability Mass Function
double CategoricalDistribution::log_pdf(double x) const {
    double prob = pdf(x);
    if (prob > 0.0) {
        return std::log(prob);
    }
    return -std::numeric_limits<double>::infinity();
}

// Cumulative Distribution Function
double CategoricalDistribution::cdf(double x) const {
    double result = 0.0;
    int max_index = static_cast<int>(std::floor(x));
    for (int i = 0; i <= max_index && i < weights_.size(); ++i) {
        result += weights_[i];
    }
    return result;
}

// Log of the Cumulative Distribution Function
double CategoricalDistribution::log_cdf(double x) const {
    double cdf_val = cdf(x);
    if (cdf_val > 0.0) {
        return std::log(cdf_val);
    }
    return -std::numeric_limits<double>::infinity();
}

// Link name (not applicable for this distribution in a standard GLM)
std::string CategoricalDistribution::link_name() const {
    return "N/A";
}

int CategoricalDistribution::sample_discrete() {
    return dist_(gen_);
}

// Link function (not applicable for this distribution in a standard GLM)
double CategoricalDistribution::link_function(double mu) const {
    throw std::logic_error("Link function not applicable for Categorical Distribution in a simple GLM context.");
}

// Mean function (not applicable for this distribution in a standard GLM)
double CategoricalDistribution::mean_function(double eta) const {
    throw std::logic_error("Mean function not applicable for Categorical Distribution in a simple GLM context.");
}
