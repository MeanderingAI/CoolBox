// LinearKernel.h
#ifndef LINEAR_KERNEL_H
#define LINEAR_KERNEL_H

#include "ml/support_vector_machine/kernel.h"

class LinearKernel : public Kernel {
public:
    double calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const override;
};

#endif // LINEAR_KERNEL_H

