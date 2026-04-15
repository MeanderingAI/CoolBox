#include "polynomial_kernel.h"

double PolynomialKernel::calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const {
    return std::pow(gamma_ * x.dot(y) + c_, degree_);
}
