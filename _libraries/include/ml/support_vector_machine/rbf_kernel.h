// RBFKernel.h
#ifndef RBF_KERNEL_H
#define RBF_KERNEL_H

#include "ml/support_vector_machine/kernel.h"

class RBFKernel : public Kernel {
public:
    RBFKernel(double gamma) : gamma_(gamma) {}
    double calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const override;
private:
    double gamma_;
};

#endif // RBF_KERNEL_H

