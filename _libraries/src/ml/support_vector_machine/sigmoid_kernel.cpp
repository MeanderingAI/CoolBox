#include "ML/support_vector_machine/sigmoid_kernel.h"
#include <cmath>

double SigmoidKernel::calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const {
    // The core sigmoid kernel calculation using Eigen's dot product
    return tanh(gamma_ * x.dot(y) + c_);
}