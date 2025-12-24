#include "ml/distribution/binomial_distribution.h"
#include "ml/distribution/util.h"
#include <random>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <numeric>

BinomialDistribution::BinomialDistribution(int t, double p)
    : dist_(t, p),gen_(std::random_device{}()) {
    if (t < 0) {
        throw std::invalid_argument("Number of trials must be non-negative.");
    }
    if (p < 0.0 || p > 1.0) {
        throw std::invalid_argument("Probability must be between 0 and 1.");
    }
}

double BinomialDistribution::pdf(double x) const {
    if (x < 0 || x > dist_.t() || std::fmod(x, 1.0) != 0.0) {
        return 0.0;
    }
    int k = static_cast<int>(x);
    double p = dist_.p();
    return combinations(dist_.t(), k) * std::pow(p, k) * std::pow(1.0 - p, dist_.t() - k);
}

double BinomialDistribution::log_pdf(double x) const {
    if (x < 0 || x > dist_.t() || std::fmod(x, 1.0) != 0.0) {
        return -std::numeric_limits<double>::infinity();
    }
    int k = static_cast<int>(x);
    double p = dist_.p();
    return std::lgamma(dist_.t() + 1) - std::lgamma(k + 1) - std::lgamma(dist_.t() - k + 1) + k * std::log(p) + (dist_.t() - k) * std::log(1.0 - p);
}

double BinomialDistribution::cdf(double x) const {
    if (x < 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i <= static_cast<int>(x) && i <= dist_.t(); ++i) {
        sum += pdf(static_cast<double>(i));
    }
    return sum;
}


double BinomialDistribution::log_cdf(double x) const
{
    if (x < 0.0) {
        return -std::numeric_limits<double>::infinity(); // log(0)
    }

    int k = static_cast<int>(std::floor(x));
    int n = dist_.t(); // number of trials
    double p = dist_.p(); // probability of success

    // Compute cumulative probability manually
    double cdf = 0.0;
    double log_term = 0.0;

    for (int i = 0; i <= k; ++i) {
        // log_combination = log(n choose i)
        double log_comb = 0.0;
        for (int j = 1; j <= i; ++j) {
            log_comb += std::log(n - j + 1) - std::log(j);
        }

        double log_prob = log_comb + i * std::log(p) + (n - i) * std::log(1 - p);
        cdf += std::exp(log_prob);
    }

    if (cdf == 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(cdf);
}

int BinomialDistribution::sample_discrete() {
    return dist_(gen_);
}

std::string BinomialDistribution::link_name() const { return "logit"; }
double BinomialDistribution::link_function(double mu) const {
    if (mu <= 0 || mu >= 1) throw std::invalid_argument("Mean must be between 0 and 1 for logit link.");
    return std::log(mu / (1.0 - mu));
}
double BinomialDistribution::mean_function(double eta) const { return logistic_function(eta); }
