#include "categorical_distribution.h"
#include <cmath>
#include <numeric>
#include <limits>
#include <stdexcept>

CategoricalDistribution::CategoricalDistribution(const std::vector<double>& weights)
    : dist_(weights.begin(), weights.end()), gen_(std::random_device{}()), weights_(weights) {
    // Normalize weights to probabilities
    double sum = std::accumulate(weights_.begin(), weights_.end(), 0.0);
    if (sum > 0.0) {
        for (auto& w : weights_) w /= sum;
    }
}

double CategoricalDistribution::pdf(double x) const {
    int k = static_cast<int>(x);
    if (k < 0 || k >= static_cast<int>(weights_.size())) return 0.0;
    return weights_[k];
}

double CategoricalDistribution::log_pdf(double x) const {
    double p = pdf(x);
    if (p <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(p);
}

double CategoricalDistribution::cdf(double x) const {
    int k = static_cast<int>(std::floor(x));
    if (k < 0) return 0.0;
    if (k >= static_cast<int>(weights_.size())) return 1.0;
    double sum = 0.0;
    for (int i = 0; i <= k; ++i) sum += weights_[i];
    return sum;
}

double CategoricalDistribution::log_cdf(double x) const {
    double c = cdf(x);
    if (c <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c);
}

int CategoricalDistribution::sample_discrete() {
    return dist_(gen_);
}

std::string CategoricalDistribution::link_name() const {
    return "none";
}

double CategoricalDistribution::link_function(double mu) const {
    return mu; // Not applicable
}

double CategoricalDistribution::mean_function(double eta) const {
    return eta; // Not applicable
}
