#include "ML/deep_learning/neural_network.h"
#include "ML/deep_learning/layer.h"
#include "ML/deep_learning/loss.h"
#include "ML/deep_learning/optimizer.h"
#include "ML/deep_learning/tensor.h"
#include <iostream>
#include <vector>
#include <memory>

using namespace ml::deep_learning;

int main() {
    std::cout << "Deep Learning Library - XOR Example" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // Create XOR dataset
    std::vector<Tensor> inputs;
    std::vector<Tensor> targets;
    
    // XOR truth table
    inputs.push_back(Tensor({1, 2}, {0.0, 0.0}));
    targets.push_back(Tensor({1, 1}, {0.0}));
    
    inputs.push_back(Tensor({1, 2}, {0.0, 1.0}));
    targets.push_back(Tensor({1, 1}, {1.0}));
    
    inputs.push_back(Tensor({1, 2}, {1.0, 0.0}));
    targets.push_back(Tensor({1, 1}, {1.0}));
    
    inputs.push_back(Tensor({1, 2}, {1.0, 1.0}));
    targets.push_back(Tensor({1, 1}, {0.0}));
    
    // Create neural network
    NeuralNetwork nn;
    
    // Add layers
    nn.add_layer(std::make_shared<DenseLayer>(2, 4));   // Input to hidden layer
    nn.add_layer(std::make_shared<ReLULayer>());        // Activation
    nn.add_layer(std::make_shared<DenseLayer>(4, 1));   // Hidden to output layer
    nn.add_layer(std::make_shared<SigmoidLayer>());     // Output activation
    
    // Set loss function
    nn.set_loss(std::make_shared<MSELoss>());
    
    // Display network architecture
    nn.summary();
    std::cout << std::endl;
    
    // Train the network
    std::cout << "Training..." << std::endl;
    nn.train(inputs, targets, 1000, 4, true);
    
    // Test the network
    std::cout << "\nTesting:" << std::endl;
    for (size_t i = 0; i < inputs.size(); ++i) {
        Tensor output = nn.predict(inputs[i]);
        std::cout << "Input: [" << inputs[i].data()[0] << ", " << inputs[i].data()[1] << "] "
                  << "-> Output: " << output.data()[0] 
                  << " (Target: " << targets[i].data()[0] << ")" << std::endl;
    }
    
    return 0;
}
