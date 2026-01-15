#include "ML/deep_learning/neural_network.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <random>

namespace ml {
namespace deep_learning {

NeuralNetwork::NeuralNetwork() : training_(true) {}

void NeuralNetwork::add_layer(std::shared_ptr<Layer> layer) {
    layers_.push_back(layer);
}

void NeuralNetwork::set_loss(std::shared_ptr<Loss> loss) {
    loss_ = loss;
}

void NeuralNetwork::set_optimizer(std::shared_ptr<Optimizer> optimizer) {
    optimizer_ = optimizer;
}

Tensor NeuralNetwork::predict(const Tensor& input) {
    Tensor output = input.clone();
    
    for (auto& layer : layers_) {
        output = layer->forward(output);
    }
    
    return output;
}

double NeuralNetwork::train_step(const Tensor& input, const Tensor& target) {
    if (!loss_) {
        throw std::runtime_error("Loss function not set");
    }
    
    // Forward pass
    Tensor output = predict(input);
    
    // Compute loss
    double loss_value = loss_->compute(output, target);
    
    // Backward pass
    Tensor loss_grad = loss_->gradient(output, target);
    backward(loss_grad);
    
    // Update parameters
    update_parameters();
    
    return loss_value;
}

void NeuralNetwork::backward(const Tensor& loss_gradient) {
    Tensor gradient = loss_gradient.clone();
    
    // Backpropagate through layers in reverse order
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        gradient = (*it)->backward(gradient);
    }
}

void NeuralNetwork::update_parameters() {
    if (optimizer_) {
        // Use optimizer (not yet fully integrated with layer parameters)
        // This would require storing parameter references in the optimizer
        for (auto& layer : layers_) {
            if (layer->has_parameters()) {
                // For now, layers update themselves
                // In a more sophisticated implementation, optimizer would handle this
                layer->update_parameters(0.01); // Default learning rate
            }
        }
    } else {
        // Simple gradient descent with default learning rate
        for (auto& layer : layers_) {
            if (layer->has_parameters()) {
                layer->update_parameters(0.01);
            }
        }
    }
}

void NeuralNetwork::train(const std::vector<Tensor>& inputs,
                         const std::vector<Tensor>& targets,
                         size_t epochs,
                         size_t batch_size,
                         bool verbose) {
    if (inputs.size() != targets.size()) {
        throw std::invalid_argument("Number of inputs and targets must match");
    }
    
    size_t num_samples = inputs.size();
    set_training(true);
    
    for (size_t epoch = 0; epoch < epochs; ++epoch) {
        double total_loss = 0.0;
        
        // Create indices for shuffling
        std::vector<size_t> indices(num_samples);
        std::iota(indices.begin(), indices.end(), 0);
        
        // Shuffle data
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::shuffle(indices.begin(), indices.end(), gen);
        
        // Train on batches
        for (size_t i = 0; i < num_samples; i += batch_size) {
            size_t current_batch_size = std::min(batch_size, num_samples - i);
            
            // For simplicity, we'll train on individual samples
            // A more sophisticated implementation would batch samples together
            for (size_t j = 0; j < current_batch_size; ++j) {
                size_t idx = indices[i + j];
                double loss = train_step(inputs[idx], targets[idx]);
                total_loss += loss;
            }
        }
        
        if (verbose) {
            std::cout << "Epoch " << (epoch + 1) << "/" << epochs 
                     << " - Loss: " << std::fixed << std::setprecision(6) 
                     << (total_loss / num_samples) << std::endl;
        }
    }
}

double NeuralNetwork::evaluate(const std::vector<Tensor>& inputs,
                               const std::vector<Tensor>& targets) {
    if (inputs.size() != targets.size()) {
        throw std::invalid_argument("Number of inputs and targets must match");
    }
    
    if (!loss_) {
        throw std::runtime_error("Loss function not set");
    }
    
    set_training(false);
    
    double total_loss = 0.0;
    for (size_t i = 0; i < inputs.size(); ++i) {
        Tensor output = predict(inputs[i]);
        total_loss += loss_->compute(output, targets[i]);
    }
    
    return total_loss / inputs.size();
}

void NeuralNetwork::set_training(bool training) {
    training_ = training;
    
    // Set training mode for all layers (especially important for dropout)
    for (auto& layer : layers_) {
        auto dropout = std::dynamic_pointer_cast<DropoutLayer>(layer);
        if (dropout) {
            dropout->set_training(training);
        }
    }
}

void NeuralNetwork::summary() const {
    std::cout << "Neural Network Summary:" << std::endl;
    std::cout << "======================" << std::endl;
    
    for (size_t i = 0; i < layers_.size(); ++i) {
        std::cout << "Layer " << (i + 1) << ": " << layers_[i]->name();
        if (layers_[i]->has_parameters()) {
            std::cout << " (trainable)";
        }
        std::cout << std::endl;
    }
    
    std::cout << "======================" << std::endl;
    
    if (loss_) {
        std::cout << "Loss: " << loss_->name() << std::endl;
    }
    
    if (optimizer_) {
        std::cout << "Optimizer: " << optimizer_->name() << std::endl;
    }
}

} // namespace deep_learning
} // namespace ml
