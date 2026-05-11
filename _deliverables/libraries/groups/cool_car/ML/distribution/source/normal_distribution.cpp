#include "normal_distribution.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

NormalDistribution::NormalDistribution(double mean, double stddev)
    : dist_(mean, stddev), gen_(std::random_device{}()), mean_(mean), stddev_(stddev) {}

double NormalDistribution::pdf(double x) const {
    double z = (x - mean_) / stddev_;
    return (1.0 / (stddev_ * std::sqrt(2.0 * M_PI))) * std::exp(-0.5 * z * z);
}

double NormalDistribution::log_pdf(double x) const {
    double z = (x - mean_) / stddev_;
    return -0.5 * std::log(2.0 * M_PI) - std::log(stddev_) - 0.5 * z * z;
}

double NormalDistribution::cdf(double x) const {
    return 0.5 * (1.0 + std::erf((x - mean_) / (stddev_ * std::sqrt(2.0))));
}

double NormalDistribution::log_cdf(double x) const {
    return std::log(cdf(x));
}

double NormalDistribution::sample() {
    return dist_(gen_);
}

std::string NormalDistribution::link_name() const {
    return "identity";
}

double NormalDistribution::link_function(double mu) const {
    return mu; // identity link
}

double NormalDistribution::mean_function(double eta) const {
    return eta; // identity inverse link
}
