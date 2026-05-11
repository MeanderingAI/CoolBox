#include <linear_kernel.h>

double LinearKernel::calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const {
    return x.dot(y);
}