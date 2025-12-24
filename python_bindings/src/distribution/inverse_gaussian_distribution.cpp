#include <inverse_gaussian_distribution.h>
#include <util.h>
#include <random>
#include <cmath>
#include <limits>
#include <stdexcept>

InverseGaussianDistribution::InverseGaussianDistribution(double mean, double shape)
    : mean_(mean), shape_(shape), gen_(std::random_device{}()) {
    if (mean <= 0 || shape <= 0) {
        throw std::invalid_argument("Mean and shape must be positive.");
    }
}

double InverseGaussianDistribution::pdf(double x) const {
    if (x <= 0) return 0.0;
    return std::sqrt(shape_ / (2.0 * M_PI * std::pow(x, 3.0))) * std::exp(-(shape_ * std::pow(x - mean_, 2.0)) / (2.0 * std::pow(mean_, 2.0) * x));
}

double InverseGaussianDistribution::log_pdf(double x) const {
    if (x <= 0) return -std::numeric_limits<double>::infinity();
    return 0.5 * (std::log(shape_) - std::log(2.0 * M_PI) - 3.0 * std::log(x)) - (shape_ * std::pow(x - mean_, 2.0)) / (2.0 * std::pow(mean_, 2.0) * x);
}

double InverseGaussianDistribution::cdf(double x) const {
    if (x <= 0) return 0.0;
    // Note: This requires the normal CDF.
    return 0.0;
}

static double standard_normal_cdf(double z) {
    return 0.5 * (1.0 + std::erf(z / std::sqrt(2.0)));
}

double InverseGaussianDistribution::log_cdf(double x) const
{
    if (x <= 0.0) {
        return -std::numeric_limits<double>::infinity(); // log(0)
    }

    double mu = mean_;
    double lambda = shape_;

    double sqrt_lambda_over_x = std::sqrt(lambda / x);

    double term1 = sqrt_lambda_over_x * (x / mu - 1.0);
    double term2 = -sqrt_lambda_over_x * (x / mu + 1.0);

    // Compute a = Phi(term1), b = exp(2*lambda/mu) * Phi(term2)
    double a = standard_normal_cdf(term1);
    double b = std::exp(2.0 * lambda / mu) * standard_normal_cdf(term2);

    // Use log-sum-exp for numerical stability
    double max_val = std::max(a, b);
    if (max_val == 0.0) {
        return -std::numeric_limits<double>::infinity();
    }

    double log_cdf_val = std::log(a / max_val + b / max_val) + std::log(max_val);
    return log_cdf_val;
}

double InverseGaussianDistribution::sample() {
    std::normal_distribution<double> normal_dist(0.0, 1.0);
    double v = normal_dist(gen_);
    double y = v * v;
    double x1 = mean_ + (mean_ * mean_ * y) / (2.0 * shape_) - (mean_ / (2.0 * shape_)) * std::sqrt(4.0 * mean_ * shape_ * y + std::pow(mean_ * y, 2.0));
    std::uniform_real_distribution<> uniform_dist(0.0, 1.0);
    double u = uniform_dist(gen_);
    if (u <= mean_ / (mean_ + x1)) {
        return x1;
    } else {
        return std::pow(mean_, 2.0) / x1;
    }
}

std::string InverseGaussianDistribution::link_name() const { return "inverse-squared"; }
double InverseGaussianDistribution::link_function(double mu) const {
    if (mu == 0) throw std::invalid_argument("Mean cannot be zero for inverse squared link.");
    return 1.0 / (mu * mu);
}
double InverseGaussianDistribution::mean_function(double eta) const {
    if (eta <= 0) throw std::invalid_argument("Linear predictor must be positive for inverse squared link.");
    return 1.0 / std::sqrt(eta);
}
