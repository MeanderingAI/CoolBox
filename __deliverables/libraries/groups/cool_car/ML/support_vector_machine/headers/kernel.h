#ifndef KERNEL_H
#define KERNEL_H

#include <vector>

class Kernel {
public:
    virtual ~Kernel() = default;
    virtual double calculate(const std::vector<double>& x, const std::vector<double>& y) const = 0;
};

#endif // KERNEL_H