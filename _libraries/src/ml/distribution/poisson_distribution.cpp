#include "ML/distribution/poisson_distribution.h"
#include "ML/distribution/util.h"
#include <random>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <numeric>
// Constructor
PoissonDistribution::PoissonDistribution(double lambda)
    : lambda_(lambda) {
    if (lambda_ <= 0) {
        throw std::invalid_argument("Lambda must be greater than 0.");
    }
}

// Probability Mass Function
double PoissonDistribution::pdf(double x) const {
    int k = static_cast<int>(x);
    if (k < 0) {
        return 0.0;
    }
    // Using std::lgamma for numerical stability instead of an external library
    double log_prob = k * std::log(lambda_) - lambda_ - std::lgamma(k + 1.0);
    return std::exp(log_prob);
}

// Log of the Probability Mass Function
double PoissonDistribution::log_pdf(double x) const {
    int k = static_cast<int>(x);
    if (k < 0) {
        return -std::numeric_limits<double>::infinity();
    }
    // Using std::lgamma for numerical stability instead of an external library
    return k * std::log(lambda_) - lambda_ - std::lgamma(k + 1.0);
}

// Cumulative Distribution Function
double PoissonDistribution::cdf(double x) const {
    int k_floor = static_cast<int>(std::floor(x));
    if (k_floor < 0) {
        return 0.0;
    }
    double sum = 0.0;
    for (int i = 0; i <= k_floor; ++i) {
        sum += pdf(static_cast<double>(i));
    }
    return sum;
}

// Log of the Cumulative Distribution Function
double PoissonDistribution::log_cdf(double x) const {
    double cdf_val = cdf(x);
    if (cdf_val > 0.0) {
        return std::log(cdf_val);
    }
    return -std::numeric_limits<double>::infinity();
}

int PoissonDistribution::sample_discrete()
{
    return dist_(gen_);
}


// Link name (Log-link is canonical for Poisson)
std::string PoissonDistribution::link_name() const {
    return "log";
}

// Link function: eta = log(mu)
double PoissonDistribution::link_function(double mu) const {
    if (mu <= 0) {
        throw std::domain_error("Mean (mu) for Poisson log-link must be positive.");
    }
    return std::log(mu);
}

// Mean function: mu = exp(eta)
double PoissonDistribution::mean_function(double eta) const {
    return std::exp(eta);
}