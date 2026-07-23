#pragma once
#include "deep_learning/layer.h"

namespace ml {
namespace deep_learning {

class RNNLayer : public Layer {
public:
    RNNLayer(int input_size, int hidden_size);
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override;
    std::string name() const override { return "RNN"; }
    bool has_parameters() const override { return true; }
private:
    int input_size_;
    int hidden_size_;
};

} // namespace deep_learning
} // namespace ml
