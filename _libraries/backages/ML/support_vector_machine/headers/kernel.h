#ifndef KERNEL_H
#define KERNEL_H

#include "mytrix_eigen_compat.hpp"

class Kernel {
public:
    virtual ~Kernel() = default;
    virtual double calculate(const Eigen::VectorXd& x, const Eigen::VectorXd& y) const = 0;
};

#endif // KERNEL_H