#include "ML/distribution/exponential_distribution.h"
#include "ML/distribution/util.h"
#include <random>
#include <cmath>
#include <limits>
#include <stdexcept>

// --- ExponentialDistribution Implementation ---

ExponentialDistribution::ExponentialDistribution(double rate)
    : rate_(rate), dist_(rate), gen_(std::random_device{}()) {
    if (rate <= 0) {
        throw std::invalid_argument("Rate must be positive.");
    }
}

double ExponentialDistribution::pdf(double x) const {
    if (x < 0) return 0.0;
    return rate_ * std::exp(-rate_ * x);
}

double ExponentialDistribution::log_pdf(double x) const {
    if (x < 0) return -std::numeric_limits<double>::infinity();
    return std::log(rate_) - rate_ * x;
}

double ExponentialDistribution::cdf(double x) const {
    if (x < 0) return 0.0;
    return 1.0 - std::exp(-rate_ * x);
}

double ExponentialDistribution::log_cdf(double x) const {
    if (x < 0.0) {
        // Exponential is 0 for x < 0
        return -std::numeric_limits<double>::infinity();
    }

    // log(1 - exp(-lambda * x)) using log1p for numerical stability
    double exp_neg = std::exp(-rate_ * x);

    if (exp_neg >= 1.0) {
        // Should not happen unless x is extremely small -> log(0)
        return -std::numeric_limits<double>::infinity();
    }

    return std::log1p(-exp_neg);
}

double ExponentialDistribution::sample() {
    return dist_(gen_);
}

std::string ExponentialDistribution::link_name() const { return "log"; }
double ExponentialDistribution::link_function(double mu) const {
    if (mu <= 0) throw std::invalid_argument("Mean must be positive for log link.");
    return std::log(mu);
}
double ExponentialDistribution::mean_function(double eta) const { return std::exp(eta); }
