#ifndef ML_DEEP_LEARNING_TEMPLATES_H
#define ML_DEEP_LEARNING_TEMPLATES_H

#include "neural_network.h"
#include <vector>
#include <string>
#include <memory>

namespace ml {
namespace deep_learning {

// Base class for all network templates
class NetworkTemplate {
public:
    virtual ~NetworkTemplate() = default;
    virtual NeuralNetwork build() = 0;
    virtual std::string name() const = 0;
};

// Multi-Layer Perceptron Template
class MLPTemplate : public NetworkTemplate {
public:
    MLPTemplate(int input_dim, const std::vector<int>& hidden_dims, int output_dim,
                const std::string& activation = "relu", double dropout_rate = 0.0,
                bool batch_norm = false);
    
    NeuralNetwork build() override;
    std::string name() const override { return "MLP"; }
    
private:
    int input_dim_;
    std::vector<int> hidden_dims_;
    int output_dim_;
    std::string activation_;
    double dropout_rate_;
    bool batch_norm_;
};

// Convolutional Neural Network Template
class CNNTemplate : public NetworkTemplate {
public:
    enum class Architecture {
        SIMPLE,    // Simple 2-3 conv layer network
        LENET,     // LeNet-5 style
        VGGLIKE,   // VGG-style with multiple conv blocks
        RESNET     // ResNet-style with residual connections
    };
    
    CNNTemplate(Architecture architecture, int num_classes,
                int input_channels = 3, int input_height = 32, int input_width = 32);
    
    NeuralNetwork build() override;
    std::string name() const override;
    
private:
    Architecture architecture_;
    int num_classes_;
    int input_channels_;
    int input_height_;
    int input_width_;
