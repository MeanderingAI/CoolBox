#include "inverse_gaussian_distribution.h"
#include <cmath>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

InverseGaussianDistribution::InverseGaussianDistribution(double mean, double shape)
    : gen_(std::random_device{}()), mean_(mean), shape_(shape) {}

double InverseGaussianDistribution::pdf(double x) const {
    if (x <= 0.0) return 0.0;
    return std::sqrt(shape_ / (2.0 * M_PI * x * x * x)) *
           std::exp(-shape_ * (x - mean_) * (x - mean_) / (2.0 * mean_ * mean_ * x));
}

double InverseGaussianDistribution::log_pdf(double x) const {
    if (x <= 0.0) return -std::numeric_limits<double>::infinity();
    return 0.5 * (std::log(shape_) - std::log(2.0 * M_PI) - 3.0 * std::log(x)) -
           shape_ * (x - mean_) * (x - mean_) / (2.0 * mean_ * mean_ * x);
}

double InverseGaussianDistribution::cdf(double x) const {
    if (x <= 0.0) return 0.0;
    double z1 = std::sqrt(shape_ / x) * (x / mean_ - 1.0);
    double z2 = -std::sqrt(shape_ / x) * (x / mean_ + 1.0);
    // Use normal CDF approximation via erfc
    auto normal_cdf = [](double z) -> double {
        return 0.5 * std::erfc(-z / std::sqrt(2.0));
    };
    return normal_cdf(z1) + std::exp(2.0 * shape_ / mean_) * normal_cdf(z2);
}

double InverseGaussianDistribution::log_cdf(double x) const {
    double c = cdf(x);
    if (c <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c);
}

double InverseGaussianDistribution::sample() {
    // Michael/Schucany/Haas method
    std::normal_distribution<double> normal(0.0, 1.0);
    double v = normal(gen_);
    double y = v * v;
    double x = mean_ + (mean_ * mean_ * y) / (2.0 * shape_) -
               (mean_ / (2.0 * shape_)) * std::sqrt(4.0 * mean_ * shape_ * y + mean_ * mean_ * y * y);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    double u = uniform(gen_);
    if (u <= mean_ / (mean_ + x)) {
        return x;
    } else {
        return mean_ * mean_ / x;
    }
}

std::string InverseGaussianDistribution::link_name() const {
    return "inverse_squared";
}

double InverseGaussianDistribution::link_function(double mu) const {
    return 1.0 / (mu * mu);
}

double InverseGaussianDistribution::mean_function(double eta) const {
    return 1.0 / std::sqrt(eta);
}
