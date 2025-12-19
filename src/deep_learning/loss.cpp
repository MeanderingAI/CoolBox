#include "deep_learning/loss.h"
#include <cmath>
#include <algorithm>

namespace ml {
namespace deep_learning {

// MSE Loss Implementation
double MSELoss::compute(const Tensor& predictions, const Tensor& targets) {
    if (predictions.size() != targets.size()) {
        throw std::invalid_argument("Predictions and targets must have the same size");
    }
    
    double sum = 0.0;
    for (size_t i = 0; i < predictions.size(); ++i) {
        double diff = predictions.data()[i] - targets.data()[i];
        sum += diff * diff;
    }
    
    return sum / predictions.size();
}

Tensor MSELoss::gradient(const Tensor& predictions, const Tensor& targets) {
    if (predictions.size() != targets.size()) {
        throw std::invalid_argument("Predictions and targets must have the same size");
    }
    
    Tensor grad = predictions - targets;
    
    // Scale by 2/n
    double scale = 2.0 / predictions.size();
    for (auto& val : grad.data()) {
        val *= scale;
    }
    
    return grad;
}

// BCE Loss Implementation
double BCELoss::compute(const Tensor& predictions, const Tensor& targets) {
    if (predictions.size() != targets.size()) {
        throw std::invalid_argument("Predictions and targets must have the same size");
    }
    
    double sum = 0.0;
    for (size_t i = 0; i < predictions.size(); ++i) {
        double pred = std::max(epsilon_, std::min(1.0 - epsilon_, predictions.data()[i]));
        double target = targets.data()[i];
        sum += -(target * std::log(pred) + (1.0 - target) * std::log(1.0 - pred));
    }
    
    return sum / predictions.size();
}

Tensor BCELoss::gradient(const Tensor& predictions, const Tensor& targets) {
    if (predictions.size() != targets.size()) {
        throw std::invalid_argument("Predictions and targets must have the same size");
    }
    
    Tensor grad(predictions.shape());
    
    for (size_t i = 0; i < predictions.size(); ++i) {
        double pred = std::max(epsilon_, std::min(1.0 - epsilon_, predictions.data()[i]));
        double target = targets.data()[i];
        grad.data()[i] = -(target / pred - (1.0 - target) / (1.0 - pred)) / predictions.size();
    }
    
    return grad;
}

// Categorical Cross-Entropy Loss Implementation
double CategoricalCrossEntropyLoss::compute(const Tensor& predictions, const Tensor& targets) {
    if (predictions.size() != targets.size()) {
        throw std::invalid_argument("Predictions and targets must have the same size");
    }
    
    if (predictions.shape().size() != 2) {
        throw std::invalid_argument("Predictions must be 2D [batch_size, num_classes]");
    }
    
    size_t batch_size = predictions.shape()[0];
    size_t num_classes = predictions.shape()[1];
    
    double sum = 0.0;
    for (size_t i = 0; i < batch_size; ++i) {
        for (size_t j = 0; j < num_classes; ++j) {
            double pred = std::max(epsilon_, std::min(1.0 - epsilon_, 
                                   predictions.data()[i * num_classes + j]));
            double target = targets.data()[i * num_classes + j];
            if (target > 0) {
                sum -= target * std::log(pred);
            }
        }
    }
    
    return sum / batch_size;
}

Tensor CategoricalCrossEntropyLoss::gradient(const Tensor& predictions, const Tensor& targets) {
    if (predictions.size() != targets.size()) {
        throw std::invalid_argument("Predictions and targets must have the same size");
    }
    
    if (predictions.shape().size() != 2) {
        throw std::invalid_argument("Predictions must be 2D [batch_size, num_classes]");
    }
    
    size_t batch_size = predictions.shape()[0];
    size_t num_classes = predictions.shape()[1];
    
    Tensor grad(predictions.shape());
    
    for (size_t i = 0; i < batch_size; ++i) {
        for (size_t j = 0; j < num_classes; ++j) {
            double pred = std::max(epsilon_, std::min(1.0 - epsilon_, 
                                   predictions.data()[i * num_classes + j]));
            double target = targets.data()[i * num_classes + j];
            grad.data()[i * num_classes + j] = -(target / pred) / batch_size;
        }
    }
    
    return grad;
}

} // namespace deep_learning
} // namespace ml
