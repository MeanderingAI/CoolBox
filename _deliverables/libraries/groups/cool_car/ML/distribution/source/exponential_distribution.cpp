#include "exponential_distribution.h"
#include <cmath>
#include <limits>

ExponentialDistribution::ExponentialDistribution(double rate)
    : dist_(rate), gen_(std::random_device{}()), rate_(rate) {}

double ExponentialDistribution::pdf(double x) const {
    if (x < 0.0) return 0.0;
    return rate_ * std::exp(-rate_ * x);
}

double ExponentialDistribution::log_pdf(double x) const {
    if (x < 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(rate_) - rate_ * x;
}

double ExponentialDistribution::cdf(double x) const {
    if (x < 0.0) return 0.0;
    return 1.0 - std::exp(-rate_ * x);
}

double ExponentialDistribution::log_cdf(double x) const {
    double c = cdf(x);
    if (c <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c);
}

double ExponentialDistribution::sample() {
    return dist_(gen_);
}

std::string ExponentialDistribution::link_name() const {
    return "log";
}

double ExponentialDistribution::link_function(double mu) const {
    return std::log(mu);
}

double ExponentialDistribution::mean_function(double eta) const {
    return std::exp(eta);
}
