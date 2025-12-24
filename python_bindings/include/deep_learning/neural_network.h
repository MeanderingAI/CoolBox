#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "layer.h"
#include "loss.h"
#include "optimizer.h"
#include "tensor.h"
#include <vector>
#include <memory>
#include <string>

namespace ml {
namespace deep_learning {

class NeuralNetwork {
public:
    NeuralNetwork();
    
    // Add layers to the network
    void add_layer(std::shared_ptr<Layer> layer);
    
    // Set loss function
    void set_loss(std::shared_ptr<Loss> loss);
    
    // Set optimizer
    void set_optimizer(std::shared_ptr<Optimizer> optimizer);
    
    // Forward pass
    Tensor predict(const Tensor& input);
    
    // Training
    double train_step(const Tensor& input, const Tensor& target);
    void train(const std::vector<Tensor>& inputs, 
               const std::vector<Tensor>& targets,
               size_t epochs,
               size_t batch_size = 32,
               bool verbose = true);
    
    // Evaluation
    double evaluate(const std::vector<Tensor>& inputs,
                   const std::vector<Tensor>& targets);
    
    // Utility
    void set_training(bool training);
    size_t num_layers() const { return layers_.size(); }
    void summary() const;
    
private:
    std::vector<std::shared_ptr<Layer>> layers_;
    std::shared_ptr<Loss> loss_;
    std::shared_ptr<Optimizer> optimizer_;
    bool training_;
    
    void backward(const Tensor& loss_gradient);
    void update_parameters();
};

} // namespace deep_learning
} // namespace ml

#endif // NEURAL_NETWORK_H
