#ifndef LAYER_H
#define LAYER_H

#include "tensor.h"
#include <memory>
#include <string>

namespace ml {
namespace deep_learning {

class Layer {
public:
    virtual ~Layer() = default;
    
    // Forward pass
    virtual Tensor forward(const Tensor& input) = 0;
    
    // Backward pass (returns gradient w.r.t. input)
    virtual Tensor backward(const Tensor& gradient) = 0;
    
    // Update parameters with optimizer
    virtual void update_parameters(double learning_rate) {}
    
    // Getters
    virtual std::string name() const = 0;
    virtual bool has_parameters() const { return false; }
    
protected:
    Tensor last_input_;
    Tensor last_output_;
};

// Dense (Fully Connected) Layer
class DenseLayer : public Layer {
public:
    DenseLayer(size_t input_size, size_t output_size);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    
    std::string name() const override { return "Dense"; }
    bool has_parameters() const override { return true; }
    
    const Tensor& weights() const { return weights_; }
    const Tensor& bias() const { return bias_; }
    
private:
    Tensor weights_;      // Shape: [input_size, output_size]
    Tensor bias_;         // Shape: [output_size]
    Tensor weight_gradient_;
    Tensor bias_gradient_;
    size_t input_size_;
    size_t output_size_;
};

// Activation Layers
class ReLULayer : public Layer {
public:
    ReLULayer() = default;
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    
    std::string name() const override { return "ReLU"; }
};

class SigmoidLayer : public Layer {
public:
    SigmoidLayer() = default;
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    
    std::string name() const override { return "Sigmoid"; }
};

class TanhLayer : public Layer {
public:
    TanhLayer() = default;
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    
    std::string name() const override { return "Tanh"; }
};

class SoftmaxLayer : public Layer {
public:
    SoftmaxLayer() = default;
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    
    std::string name() const override { return "Softmax"; }
};

// Dropout Layer
class DropoutLayer : public Layer {
public:
    DropoutLayer(double dropout_rate);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    
    void set_training(bool training) { training_ = training; }
    std::string name() const override { return "Dropout"; }
    
private:
    double dropout_rate_;
    bool training_;
    Tensor mask_;
};

} // namespace deep_learning
} // namespace ml

#endif // LAYER_H
