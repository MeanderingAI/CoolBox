#include "linear_kernel.h"
#include <numeric>

double LinearKernel::calculate(const std::vector<double>& x, const std::vector<double>& y) const {
    double result = 0.0;
    for (size_t i = 0; i < x.size() && i < y.size(); ++i) {
        result += x[i] * y[i];
    }
    return result;
}
