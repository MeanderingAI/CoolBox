#include <laplace_distribution.h>
#include <util.h>
#include <random>
#include <cmath>
#include <limits>
#include <stdexcept>

// --- LaplaceDistribution Implementation ---

LaplaceDistribution::LaplaceDistribution(double loc, double scale)
    : location_(loc), scale_(scale), gen_(std::random_device{}()) {
    if (scale <= 0) {
        throw std::invalid_argument("Scale parameter must be positive.");
    }
}

double LaplaceDistribution::pdf(double x) const {
    return (1.0 / (2.0 * scale_)) * std::exp(-std::abs(x - location_) / scale_);
}

double LaplaceDistribution::log_pdf(double x) const {
    return std::log(0.5) - std::log(scale_) - std::abs(x - location_) / scale_;
}

double LaplaceDistribution::cdf(double x) const {
    if (x < location_) {
        return 0.5 * std::exp((x - location_) / scale_);
    } else {
        return 1.0 - 0.5 * std::exp(-(x - location_) / scale_);
    }
}
// Log of the Cumulative Distribution Function (Log-CDF)
double LaplaceDistribution::log_cdf(double x) const {
    if (x <= location_) {
        return std::log(0.5) + (x - location_) / scale_;
    } else {
        return std::log(1.0 - 0.5 * std::exp(-(x - location_) / scale_));
    }
}

double LaplaceDistribution::sample() {
    std::uniform_real_distribution<> uniform_dist(0.0, 1.0);
    double u = uniform_dist(gen_) - 0.5;
    return location_ - scale_ * std::copysign(1.0, u) * std::log(1.0 - 2.0 * std::abs(u));
}

std::string LaplaceDistribution::link_name() const { return "identity"; }
double LaplaceDistribution::link_function(double mu) const { return mu; }
double LaplaceDistribution::mean_function(double eta) const { return eta; }
