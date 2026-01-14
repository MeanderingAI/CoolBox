#include "ML/distribution/normal_distribution.h"
#include "ML/distribution/util.h"
#include <random>
#include <cmath>
#include <limits>
#include <stdexcept>

// --- NormalDistribution Implementation ---

NormalDistribution::NormalDistribution(double mean, double stddev)
    : mean_(mean), stddev_(stddev), dist_(mean, stddev), gen_(std::random_device{}()) {
    if (stddev <= 0) {
        throw std::invalid_argument("Standard deviation must be positive.");
    }
}

double NormalDistribution::pdf(double x) const {
    double exponent = -0.5 * std::pow((x - mean_) / stddev_, 2.0);
    return (1.0 / (stddev_ * std::sqrt(2.0 * M_PI))) * std::exp(exponent);
}

double NormalDistribution::log_pdf(double x) const {
    return -0.5 * std::log(2.0 * M_PI) - std::log(stddev_) - 0.5 * std::pow((x - mean_) / stddev_, 2.0);
}

double NormalDistribution::cdf(double x) const {
    return 0.5 * (1.0 + std::erf((x - mean_) / (stddev_ * std::sqrt(2.0))));
}

double NormalDistribution::log_cdf(double x) const {
    double z = (x - mean_) / (stddev_ * std::sqrt(2.0));

    // For numerical stability in tails
    if (z < -6.0) {
        // CDF ~ 0 -> log(CDF) ~ -infinity
        return -std::numeric_limits<double>::infinity();
    }
    else if (z > 6.0) {
        // CDF ~ 1 -> log(CDF) ~ 0
        return 0.0;
    }

    // Use complementary error function for better stability
    double cdf = 0.5 * std::erfc(-z);

    // Ensure cdf > 0 to avoid log(0)
    if (cdf <= 0.0) {
        return -std::numeric_limits<double>::infinity();
    }

    return std::log(cdf);
}

double NormalDistribution::sample() {
    return dist_(gen_);
}

std::string NormalDistribution::link_name() const { return "identity"; }
double NormalDistribution::link_function(double mu) const { return mu; }
double NormalDistribution::mean_function(double eta) const { return eta; }
