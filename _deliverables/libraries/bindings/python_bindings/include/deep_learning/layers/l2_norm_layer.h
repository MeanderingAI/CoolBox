#pragma once
#include "DL/layers/include/layer.h"

namespace ml {
namespace deep_learning {

class L2NormLayer : public Layer {
public:
    L2NormLayer() = default;
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override {}
    std::string name() const override { return "L2Norm"; }
    bool has_parameters() const override { return false; }
};

} // namespace deep_learning
} // namespace ml
