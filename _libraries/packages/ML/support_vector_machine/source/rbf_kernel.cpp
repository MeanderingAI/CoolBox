#include "rbf_kernel.h"
#include <cmath>

double RBFKernel::calculate(const std::vector<double>& x, const std::vector<double>& y) const {
    double sum = 0.0;
    for (size_t i = 0; i < x.size() && i < y.size(); ++i) {
        double diff = x[i] - y[i];
        sum += diff * diff;
    }
    return std::exp(-gamma_ * sum);
}
