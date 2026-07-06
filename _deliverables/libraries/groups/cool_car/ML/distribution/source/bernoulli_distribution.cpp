#include "bernoulli_distribution.h"
#include "util.h"
#include <cmath>
#include <limits>

BernoulliDistribution::BernoulliDistribution(double p)
    : dist_(p), gen_(std::random_device{}()) {}

double BernoulliDistribution::pdf(double x) const {
    int k = static_cast<int>(x);
    if (k == 1) return dist_.p();
    if (k == 0) return 1.0 - dist_.p();
    return 0.0;
}

double BernoulliDistribution::log_pdf(double x) const {
    double p = pdf(x);
    if (p <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(p);
}

double BernoulliDistribution::cdf(double x) const {
    if (x < 0.0) return 0.0;
    if (x < 1.0) return 1.0 - dist_.p();
    return 1.0;
}

double BernoulliDistribution::log_cdf(double x) const {
    double c = cdf(x);
    if (c <= 0.0) return -std::numeric_limits<double>::infinity();
    return std::log(c);
}

int BernoulliDistribution::sample_discrete() {
    return dist_(gen_) ? 1 : 0;
}

std::string BernoulliDistribution::link_name() const {
    return "logit";
}

double BernoulliDistribution::link_function(double mu) const {
    return std::log(mu / (1.0 - mu)); // logit
}

double BernoulliDistribution::mean_function(double eta) const {
    return logistic_function(eta); // sigmoid
}
