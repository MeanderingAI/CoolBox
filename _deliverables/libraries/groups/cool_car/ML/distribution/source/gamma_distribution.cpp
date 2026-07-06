#include "gamma_distribution.h"
#include <cmath>
#include <limits>

GammaDistribution::GammaDistribution(double shape, double rate)
    : dist_(shape, 1.0 / rate), gen_(std::random_device{}()), shape_(shape), rate_(rate) {}

double GammaDistribution::pdf(double x) const {
    if (x <= 0.0) return 0.0;
    return (std::pow(rate_, shape_) / std::tgamma(shape_)) *
           std::pow(x, shape_ - 1.0) * std::exp(-rate_ * x);
}

double GammaDistribution::log_pdf(double x) const {
    if (x <= 0.0) return -std::numeric_limits<double>::infinity();
    return shape_ * std::log(rate_) - std::lgamma(shape_) +
           (shape_ - 1.0) * std::log(x) - rate_ * x;
}

double GammaDistribution::cdf(double x) const {
    if (x <= 0.0) return 0.0;
    // Use the regularized lower incomplete gamma function
    // For a simple approximation, sum the series
    // P(a, x) = gamma(a, x) / Gamma(a)
    // Use a numerical approach
    double sum = 0.0;
    double term = 1.0 / shape_;
    double val = rate_ * x;
    for (int n = 0; n < 200; ++n) {
        sum += term;
        term *= val / (shape_ + n + 1.0);
        if (std::abs(term) < 1e-15) break;
    }
    return sum * std::exp(-val + shape_ * std::log(val) - std::lgamma(shape_));
}

double GammaDistribution::log_cdf(double x) const {
    double c = cdf(x);
    if (c <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c);
}

double GammaDistribution::sample() {
    return dist_(gen_);
}

std::string GammaDistribution::link_name() const {
    return "inverse";
}

double GammaDistribution::link_function(double mu) const {
    return 1.0 / mu; // inverse link
}

double GammaDistribution::mean_function(double eta) const {
    return 1.0 / eta; // inverse
}
