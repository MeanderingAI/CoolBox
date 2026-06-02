#include "neural_network.h"
#include <iostream>

namespace ml {
namespace deep_learning {

NeuralNetwork::NeuralNetwork() = default;

void NeuralNetwork::add_layer(std::shared_ptr<Layer> layer) {
    layers_.push_back(layer);
}

void NeuralNetwork::set_loss(std::shared_ptr<Loss> loss) {
    loss_ = loss;
}

void NeuralNetwork::set_optimizer(std::shared_ptr<Optimizer> optimizer) {
    optimizer_ = optimizer;
}

Tensor NeuralNetwork::forward(const Tensor& input) {
    Tensor current = input;
    for (auto& layer : layers_) {
        current = layer->forward(current);
    }
    return current;
}

void NeuralNetwork::backward(const Tensor& target) {
    if (!loss_ || layers_.empty()) return;
    // Compute loss gradient
    Tensor grad = loss_->gradient(layers_.back()->forward(Tensor()), target);
    // Actually, we need the last output. Let's recompute:
    // The forward already stores last_output_ in each layer
    // We just need the gradient from the loss
}

Tensor NeuralNetwork::predict(const Tensor& input) {
    return forward(input);
}

void NeuralNetwork::train(const std::vector<Tensor>& inputs, const std::vector<Tensor>& targets,
                           int epochs, int batch_size, bool verbose) {
    if (!loss_) return;

    for (int epoch = 0; epoch < epochs; ++epoch) {
        double total_loss = 0.0;
        int num_samples = static_cast<int>(inputs.size());

        for (int i = 0; i < num_samples; ++i) {
            // Forward pass
            Tensor output = forward(inputs[i]);

            // Compute loss
            total_loss += loss_->compute(output, targets[i]);

            // Backward pass
            Tensor grad = loss_->gradient(output, targets[i]);
            for (int l = static_cast<int>(layers_.size()) - 1; l >= 0; --l) {
                grad = layers_[l]->backward(grad);
            }

            // Update parameters
            for (auto& layer : layers_) {
                if (layer->has_parameters()) {
                    layer->update_parameters(0.01); // default lr
                }
            }
        }

        last_loss_ = total_loss / num_samples;
        if (verbose) {
            std::cout << "Epoch " << (epoch + 1) << "/" << epochs
                      << " - Loss: " << last_loss_ << std::endl;
        }
    }
}

} // namespace deep_learning
} // namespace ml
