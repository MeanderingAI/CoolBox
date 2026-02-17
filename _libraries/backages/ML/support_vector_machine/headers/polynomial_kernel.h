#ifndef POLYNOMIAL_KERNEL_H
#define POLYNOMIAL_KERNEL_H

#include "ml/support_vector_machine/kernel.h"
#include <Eigen/Dense>
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