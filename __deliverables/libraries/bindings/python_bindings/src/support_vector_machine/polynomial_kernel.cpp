#include <polynomial_kernel.h>

double PolynomialKernel::calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const {
    double dot_product = x.dot(y);
    return std::pow(gamma_ * dot_product + c_, degree_);
}