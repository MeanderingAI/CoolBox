#ifndef LOSS_H
#define LOSS_H

#include "tensor.h"
#include <string>

namespace ml {
namespace deep_learning {

class Loss {
public:
    virtual ~Loss() = default;
    
    // Compute loss value
    virtual double compute(const Tensor& predictions, const Tensor& targets) = 0;
    
    // Compute gradient of loss w.r.t. predictions
    virtual Tensor gradient(const Tensor& predictions, const Tensor& targets) = 0;
    
    virtual std::string name() const = 0;
};

// Mean Squared Error Loss
class MSELoss : public Loss {
public:
    MSELoss() = default;
    
    double compute(const Tensor& predictions, const Tensor& targets) override;
    Tensor gradient(const Tensor& predictions, const Tensor& targets) override;
    
    std::string name() const override { return "MSE"; }
};

// Binary Cross-Entropy Loss
class BCELoss : public Loss {
public:
    BCELoss() = default;
    
    double compute(const Tensor& predictions, const Tensor& targets) override;
    Tensor gradient(const Tensor& predictions, const Tensor& targets) override;
    
    std::string name() const override { return "BCE"; }
    
private:
    const double epsilon_ = 1e-7; // For numerical stability
};

// Categorical Cross-Entropy Loss
class CategoricalCrossEntropyLoss : public Loss {
public:
    CategoricalCrossEntropyLoss() = default;
    
    double compute(const Tensor& predictions, const Tensor& targets) override;
    Tensor gradient(const Tensor& predictions, const Tensor& targets) override;
    
    std::string name() const override { return "CategoricalCrossEntropy"; }
    
private:
    const double epsilon_ = 1e-7; // For numerical stability
};

} // namespace deep_learning
} // namespace ml

#endif // LOSS_H
