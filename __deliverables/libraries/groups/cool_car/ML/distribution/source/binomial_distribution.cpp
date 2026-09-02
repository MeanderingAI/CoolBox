#include "binomial_distribution.h"
#include "util.h"
#include <cmath>
#include <limits>

BinomialDistribution::BinomialDistribution(int t, double p)
    : dist_(t, p), gen_(std::random_device{}()) {}

double BinomialDistribution::pdf(double x) const {
    int k = static_cast<int>(x);
    int n = dist_.t();
    double p = dist_.p();
    if (k < 0 || k > n) return 0.0;
    return static_cast<double>(combinations(n, k)) * std::pow(p, k) * std::pow(1.0 - p, n - k);
}

double BinomialDistribution::log_pdf(double x) const {
    double p = pdf(x);
    if (p <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(p);
}

double BinomialDistribution::cdf(double x) const {
    int k = static_cast<int>(std::floor(x));
    int n = dist_.t();
    double p_val = dist_.p();
    if (k < 0) return 0.0;
    if (k >= n) return 1.0;
    double sum = 0.0;
    for (int i = 0; i <= k; ++i) {
        sum += static_cast<double>(combinations(n, i)) * std::pow(p_val, i) * std::pow(1.0 - p_val, n - i);
    }
    return sum;
}

double BinomialDistribution::log_cdf(double x) const {
    double c = cdf(x);
    if (c <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c);
}

int BinomialDistribution::sample_discrete() {
    return dist_(gen_);
}

std::string BinomialDistribution::link_name() const {
    return "logit";
}

double BinomialDistribution::link_function(double mu) const {
    return std::log(mu / (1.0 - mu)); // logit
}

double BinomialDistribution::mean_function(double eta) const {
    return 1.0 / (1.0 + std::exp(-eta)); // logistic
}
