#include "sigmoid_kernel.h"
#include <cmath>

double SigmoidKernel::calculate(const std::vector<double>& x, const std::vector<double>& y) const {
    double dot = 0.0;
    for (size_t i = 0; i < x.size() && i < y.size(); ++i) {
        dot += x[i] * y[i];
    }
    return std::tanh(gamma_ * dot + c_);
}
