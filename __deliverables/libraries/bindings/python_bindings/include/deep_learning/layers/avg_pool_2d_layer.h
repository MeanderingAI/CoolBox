#pragma once
#include "DL/layers/include/layer.h"

namespace ml {
namespace deep_learning {

class AvgPool2DLayer : public Layer {
public:
    AvgPool2DLayer(int kernel_size, int stride);
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& gradient) override;
    void update_parameters(double learning_rate) override {}
    std::string name() const override { return "AvgPool2D"; }
    bool has_parameters() const override { return false; }
private:
    int kernel_size_;
    int stride_;
};

} // namespace deep_learning
} // namespace ml
