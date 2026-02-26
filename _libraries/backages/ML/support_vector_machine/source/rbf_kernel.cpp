#include "rbf_kernel.h"
#include <cmath>

double RBFKernel::calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const {
    return std::exp(-gamma_ * (x - y).squaredNorm());
}
