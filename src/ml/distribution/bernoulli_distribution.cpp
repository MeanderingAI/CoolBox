#include "ml/distribution/bernoulli_distribution.h"
#include "ml/distribution/util.h"
#include <random>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <numeric>


BernoulliDistribution::BernoulliDistribution(double p)
    : dist_(p), gen_(std::random_device{}()) {
    if (p < 0.0 || p > 1.0) {
        throw std::invalid_argument("Probability must be between 0 and 1.");
    }
}

double BernoulliDistribution::pdf(double x) const {
    if (x == 1.0) {
        return dist_.p();
    } else if (x == 0.0) {
        return 1.0 - dist_.p();
    }
    return 0.0;
}

double BernoulliDistribution::log_pdf(double x) const {
    if (x == 1.0) {
        return std::log(dist_.p());
    } else if (x == 0.0) {
        return std::log(1.0 - dist_.p());
    }
    return -std::numeric_limits<double>::infinity();
}

double BernoulliDistribution::cdf(double x) const {
    if (x < 0.0) {
        return 0.0;
    } else if (x < 1.0) {
        return 1.0 - dist_.p();
    } else {
        return 1.0;
    }
}

// Log of the Cumulative Distribution Function
double BernoulliDistribution::log_cdf(double x) const {
    double cdf_val = cdf(x);
    if (cdf_val > 0.0) {
        return std::log(cdf_val);
    }
    return -std::numeric_limits<double>::infinity();
}

int BernoulliDistribution::sample_discrete() {
    return dist_(gen_);
}

std::string BernoulliDistribution::link_name() const { return "logit"; }
double BernoulliDistribution::link_function(double mu) const {
    if (mu <= 0 || mu >= 1) throw std::invalid_argument("Mean must be between 0 and 1 for logit link.");
    return std::log(mu / (1.0 - mu));
}
double BernoulliDistribution::mean_function(double eta) const { return logistic_function(eta); }
