#include "laplace_distribution.h"
#include <cmath>
#include <limits>

LaplaceDistribution::LaplaceDistribution(double loc, double scale)
    : gen_(std::random_device{}()), location_(loc), scale_(scale) {}

double LaplaceDistribution::pdf(double x) const {
    return (1.0 / (2.0 * scale_)) * std::exp(-std::abs(x - location_) / scale_);
}

double LaplaceDistribution::log_pdf(double x) const {
    return -std::log(2.0 * scale_) - std::abs(x - location_) / scale_;
}

double LaplaceDistribution::cdf(double x) const {
    if (x < location_) {
        return 0.5 * std::exp((x - location_) / scale_);
    } else {
        return 1.0 - 0.5 * std::exp(-(x - location_) / scale_);
    }
}

double LaplaceDistribution::log_cdf(double x) const {
    double c = cdf(x);
    if (c <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c);
}

double LaplaceDistribution::sample() {
    std::uniform_real_distribution<double> uniform(-0.5, 0.5);
    double u = uniform(gen_);
    return location_ - scale_ * ((u > 0) - (u < 0)) * std::log(1.0 - 2.0 * std::abs(u));
}

std::string LaplaceDistribution::link_name() const {
    return "identity";
}

double LaplaceDistribution::link_function(double mu) const {
    return mu;
}

double LaplaceDistribution::mean_function(double eta) const {
    return eta;
}
