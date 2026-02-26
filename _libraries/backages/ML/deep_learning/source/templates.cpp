#include "templates.h"

namespace ml {
namespace deep_learning {

// MLPTemplate
MLPTemplate::MLPTemplate(int input_dim, const std::vector<int>& hidden_dims, int output_dim,
                         const std::string& activation, double dropout_rate, bool batch_norm)
    : input_dim_(input_dim), hidden_dims_(hidden_dims), output_dim_(output_dim),
      activation_(activation), dropout_rate_(dropout_rate), batch_norm_(batch_norm) {}

NeuralNetwork MLPTemplate::build() {
    NeuralNetwork net;
    int prev_dim = input_dim_;
    for (int hidden_dim : hidden_dims_) {
        net.add_layer(std::make_shared<DenseLayer>(prev_dim, hidden_dim));
        if (activation_ == "relu") {
            net.add_layer(std::make_shared<ReLULayer>());
        }
        prev_dim = hidden_dim;
    }
    net.add_layer(std::make_shared<DenseLayer>(prev_dim, output_dim_));
    return net;
}

// CNNTemplate
CNNTemplate::CNNTemplate(Architecture architecture, int num_classes,
                         int input_channels, int input_height, int input_width)
    : architecture_(architecture), num_classes_(num_classes),
      input_channels_(input_channels), input_height_(input_height), input_width_(input_width) {}

NeuralNetwork CNNTemplate::build() {
    NeuralNetwork net;
    // Simple MLP fallback since we don't have Conv layers yet
    int input_size = input_channels_ * input_height_ * input_width_;
    net.add_layer(std::make_shared<DenseLayer>(input_size, 128));
    net.add_layer(std::make_shared<ReLULayer>());
    net.add_layer(std::make_shared<DenseLayer>(128, num_classes_));
    return net;
}

std::string CNNTemplate::name() const {
    switch (architecture_) {
        case Architecture::SIMPLE: return "SimpleCNN";
        case Architecture::LENET: return "LeNet";
        case Architecture::VGGLIKE: return "VGGLike";
        case Architecture::RESNET: return "ResNet";
        default: return "CNN";
    }
}

} // namespace deep_learning
} // namespace ml
