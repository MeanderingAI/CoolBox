#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "tensor.h"
#include <string>
#include <memory>

namespace ml {
namespace deep_learning {

class Optimizer {
public:
    virtual ~Optimizer() = default;
    
    virtual void step(Tensor& parameters, const Tensor& gradients) = 0;
    virtual std::string name() const = 0;
    virtual void reset() {}
};

// Stochastic Gradient Descent
class SGD : public Optimizer {
public:
    SGD(double learning_rate, double momentum = 0.0);
    
    void step(Tensor& parameters, const Tensor& gradients) override;
    std::string name() const override { return "SGD"; }
    void reset() override;
    
private:
    double learning_rate_;
    double momentum_;
    Tensor velocity_;
    bool initialized_;
};

// Adam Optimizer
class Adam : public Optimizer {
public:
    Adam(double learning_rate = 0.001, double beta1 = 0.9, double beta2 = 0.999, double epsilon = 1e-8);
    
    void step(Tensor& parameters, const Tensor& gradients) override;
    std::string name() const override { return "Adam"; }
    void reset() override;
    
private:
    double learning_rate_;
    double beta1_;
    double beta2_;
    double epsilon_;
    Tensor m_; // First moment estimate
    Tensor v_; // Second moment estimate
    size_t t_; // Time step
    bool initialized_;
};

// RMSprop Optimizer
class RMSprop : public Optimizer {
public:
    RMSprop(double learning_rate = 0.001, double decay = 0.9, double epsilon = 1e-8);
    
    void step(Tensor& parameters, const Tensor& gradients) override;
    std::string name() const override { return "RMSprop"; }
    void reset() override;
    
private:
    double learning_rate_;
    double decay_;
    double epsilon_;
    Tensor cache_;
    bool initialized_;
};

} // namespace deep_learning
} // namespace ml

#endif // OPTIMIZER_H
