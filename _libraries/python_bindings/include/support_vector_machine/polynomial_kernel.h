#ifndef POLYNOMIAL_KERNEL_H
#define POLYNOMIAL_KERNEL_H

#include "kernel.h"
#include "mytrix_eigen_compat.hpp"
// All matrix/vector types now use mytrix::Matrix, mytrix::Vector, etc.
#include <cmath>

class PolynomialKernel : public Kernel {
public:
    PolynomialKernel(double gamma, double c, int degree) : gamma_(gamma), c_(c), degree_(degree) {}
    double calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const override;
private:
    double gamma_;
    double c_;
    int degree_;
};

#endif // POLYNOMIAL_H