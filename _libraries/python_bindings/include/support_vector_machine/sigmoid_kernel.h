#ifndef SIGMOID_KERNEL_H
#define SIGMOID_KERNEL_H

#include <kernel.h>
#include <Eigen/Dense>

class SigmoidKernel : public Kernel {
public:
    SigmoidKernel(double gamma, double c) : gamma_(gamma), c_(c) {}
    double calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const override;
private:
    double gamma_;
    double c_;
};

#endif // SIGMOID_KERNEL_H