#include "sigmoid_kernel.h"
#include <cmath>

double SigmoidKernel::calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const {
    return std::tanh(gamma_ * x.dot(y) + c_);
}
