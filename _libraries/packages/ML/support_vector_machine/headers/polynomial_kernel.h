#ifndef POLYNOMIAL_KERNEL_H
#define POLYNOMIAL_KERNEL_H

#include "kernel.h"
#include <vector>
#include <cmath>

class PolynomialKernel : public Kernel {
public:
    PolynomialKernel(double gamma, double c, int degree) : gamma_(gamma), c_(c), degree_(degree) {}
    double calculate(const std::vector<double>& x, const std::vector<double>& y) const override;
private:
    double gamma_;
    double c_;
    int degree_;
};

#endif // POLYNOMIAL_H