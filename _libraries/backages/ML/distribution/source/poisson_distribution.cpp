#include "poisson_distribution.h"
#include "util.h"
#include <cmath>
#include <limits>

PoissonDistribution::PoissonDistribution(double lambda)
    : dist_(lambda), gen_(std::random_device{}()), lambda_(lambda) {}

double PoissonDistribution::pdf(double x) const {
    int k = static_cast<int>(x);
    if (k < 0) return 0.0;
    return std::exp(-lambda_) * std::pow(lambda_, k) / static_cast<double>(factorial(k));
}

double PoissonDistribution::log_pdf(double x) const {
    int k = static_cast<int>(x);
    if (k < 0) return -std::numeric_limits<double>::infinity();
    double log_fact = 0.0;
    for (int i = 2; i <= k; ++i) log_fact += std::log(static_cast<double>(i));
    return -lambda_ + k * std::log(lambda_) - log_fact;
}

double PoissonDistribution::cdf(double x) const {
    int k = static_cast<int>(std::floor(x));
    if (k < 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i <= k; ++i) {
        sum += std::exp(-lambda_) * std::pow(lambda_, i) / static_cast<double>(factorial(i));
    }
    return sum;
}

double PoissonDistribution::log_cdf(double x) const {
    double c = cdf(x);
    if (c <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c);
}

int PoissonDistribution::sample_discrete() {
    return dist_(gen_);
}

std::string PoissonDistribution::link_name() const {
    return "log";
}

double PoissonDistribution::link_function(double mu) const {
    return std::log(mu);
}

double PoissonDistribution::mean_function(double eta) const {
    return std::exp(eta);
}
