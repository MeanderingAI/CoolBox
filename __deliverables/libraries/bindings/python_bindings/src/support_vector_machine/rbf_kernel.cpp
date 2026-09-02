
#include <rbf_kernel.h>

double RBFKernel::calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const {
    double squared_distance = (x - y).squaredNorm();
    return exp(-gamma_ * squared_distance);
}