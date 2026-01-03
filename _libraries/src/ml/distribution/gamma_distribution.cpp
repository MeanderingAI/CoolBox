#include "ml/distribution/gamma_distribution.h"
#include "ml/distribution/util.h"
#include <random>
#include <cmath>
#include <limits>
#include <gsl/gsl_cdf.h> // GSL header for CDF functions


GammaDistribution::GammaDistribution(double shape, double rate)
    : shape_(shape), rate_(rate), dist_(shape, 1.0 / rate), gen_(std::random_device{}()) {
    if (shape <= 0 || rate <= 0) {
        throw std::invalid_argument("Shape and rate must be positive.");
    }
}

double GammaDistribution::pdf(double x) const {
    if (x < 0) return 0.0;
    return (std::pow(rate_, shape_) / std::tgamma(shape_)) * std::pow(x, shape_ - 1.0) * std::exp(-rate_ * x);
}

double GammaDistribution::log_pdf(double x) const {
    if (x < 0) return -std::numeric_limits<double>::infinity();
    return shape_ * std::log(rate_) - std::lgamma(shape_) + (shape_ - 1.0) * std::log(x) - rate_ * x;
}

// Cumulative Distribution Function (CDF)
// This function uses the GSL library to calculate the regularized incomplete gamma function.
// GSL's gamma distribution CDF (gsl_cdf_gamma_P) takes the scale parameter (k)
// which is the inverse of the rate parameter (beta) used in our class.
double GammaDistribution::cdf(double x) const {
    if (x < 0) return 0.0;
    return gsl_cdf_gamma_P(x, shape_, 1.0 / rate_);
}

// Log of the Cumulative Distribution Function (Log-CDF)
// This calculates the log-transformed CDF using the cdf() method.
double GammaDistribution::log_cdf(double x) const {
    double cdf_val = cdf(x);
    if (cdf_val <= 0.0) {
        return -std::numeric_limits<double>::infinity();
    }
    return std::log(cdf_val);
}
double GammaDistribution::sample() {
    return dist_(gen_);
}

std::string GammaDistribution::link_name() const { return "inverse"; }
double GammaDistribution::link_function(double mu) const {
    if (mu == 0) throw std::invalid_argument("Mean cannot be zero for inverse link.");
    return 1.0 / mu;
}
double GammaDistribution::mean_function(double eta) const {
    if (eta == 0) throw std::invalid_argument("Linear predictor cannot be zero for inverse link.");
    return 1.0 / eta;
}
