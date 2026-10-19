#include "loss.h"
#include <cmath>
#include <algorithm>

namespace ml {
namespace deep_learning {

// MSE Loss
double MSELoss::compute(const Tensor& predictions, const Tensor& targets) {
    double sum = 0.0;
    for (size_t i = 0; i < predictions.size(); ++i) {
        double diff = predictions.data()[i] - targets.data()[i];
        sum += diff * diff;
    }
    return sum / static_cast<double>(predictions.size());
}

Tensor MSELoss::gradient(const Tensor& predictions, const Tensor& targets) {
    Tensor grad(predictions.shape());
    double n = static_cast<double>(predictions.size());
    for (size_t i = 0; i < predictions.size(); ++i) {
        grad.data()[i] = 2.0 * (predictions.data()[i] - targets.data()[i]) / n;
    }
    return grad;
}

// BCE Loss
double BCELoss::compute(const Tensor& predictions, const Tensor& targets) {
    double sum = 0.0;
    for (size_t i = 0; i < predictions.size(); ++i) {
        double p = std::clamp(predictions.data()[i], epsilon_, 1.0 - epsilon_);
        double t = targets.data()[i];
        sum += -(t * std::log(p) + (1.0 - t) * std::log(1.0 - p));
    }
    return sum / static_cast<double>(predictions.size());
}

Tensor BCELoss::gradient(const Tensor& predictions, const Tensor& targets) {
    Tensor grad(predictions.shape());
    double n = static_cast<double>(predictions.size());
    for (size_t i = 0; i < predictions.size(); ++i) {
        double p = std::clamp(predictions.data()[i], epsilon_, 1.0 - epsilon_);
        double t = targets.data()[i];
        grad.data()[i] = (-t / p + (1.0 - t) / (1.0 - p)) / n;
    }
    return grad;
}

// Categorical Cross-Entropy Loss
double CategoricalCrossEntropyLoss::compute(const Tensor& predictions, const Tensor& targets) {
    double sum = 0.0;
    for (size_t i = 0; i < predictions.size(); ++i) {
        double p = std::max(predictions.data()[i], epsilon_);
        sum += -targets.data()[i] * std::log(p);
    }
    return sum / static_cast<double>(predictions.shape()[0]); // Average over batch
}

Tensor CategoricalCrossEntropyLoss::gradient(const Tensor& predictions, const Tensor& targets) {
    Tensor grad(predictions.shape());
    double n = static_cast<double>(predictions.shape()[0]);
    for (size_t i = 0; i < predictions.size(); ++i) {
        double p = std::max(predictions.data()[i], epsilon_);
        grad.data()[i] = -targets.data()[i] / p / n;
    }
    return grad;
}

} // namespace deep_learning
} // namespace ml
