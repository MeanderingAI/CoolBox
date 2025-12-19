#include "deep_learning/optimizer.h"
#include <cmath>

namespace ml {
namespace deep_learning {

// SGD Implementation
SGD::SGD(double learning_rate, double momentum)
    : learning_rate_(learning_rate), momentum_(momentum), initialized_(false) {
    if (learning_rate <= 0.0) {
        throw std::invalid_argument("Learning rate must be positive");
    }
    if (momentum < 0.0 || momentum >= 1.0) {
        throw std::invalid_argument("Momentum must be in [0, 1)");
    }
}

void SGD::step(Tensor& parameters, const Tensor& gradients) {
    if (parameters.size() != gradients.size()) {
        throw std::invalid_argument("Parameters and gradients must have the same size");
    }
    
    if (!initialized_) {
        velocity_ = Tensor(parameters.shape(), 0.0);
        initialized_ = true;
    }
    
    if (momentum_ > 0.0) {
        // Update velocity: v = momentum * v - learning_rate * gradient
        for (size_t i = 0; i < parameters.size(); ++i) {
            velocity_.data()[i] = momentum_ * velocity_.data()[i] - 
                                 learning_rate_ * gradients.data()[i];
            parameters.data()[i] += velocity_.data()[i];
        }
    } else {
        // Simple SGD
        for (size_t i = 0; i < parameters.size(); ++i) {
            parameters.data()[i] -= learning_rate_ * gradients.data()[i];
        }
    }
}

void SGD::reset() {
    initialized_ = false;
    velocity_ = Tensor();
}

// Adam Implementation
Adam::Adam(double learning_rate, double beta1, double beta2, double epsilon)
    : learning_rate_(learning_rate), beta1_(beta1), beta2_(beta2), 
      epsilon_(epsilon), t_(0), initialized_(false) {
    if (learning_rate <= 0.0) {
        throw std::invalid_argument("Learning rate must be positive");
    }
    if (beta1 < 0.0 || beta1 >= 1.0) {
        throw std::invalid_argument("Beta1 must be in [0, 1)");
    }
    if (beta2 < 0.0 || beta2 >= 1.0) {
        throw std::invalid_argument("Beta2 must be in [0, 1)");
    }
}

void Adam::step(Tensor& parameters, const Tensor& gradients) {
    if (parameters.size() != gradients.size()) {
        throw std::invalid_argument("Parameters and gradients must have the same size");
    }
    
    if (!initialized_) {
        m_ = Tensor(parameters.shape(), 0.0);
        v_ = Tensor(parameters.shape(), 0.0);
        initialized_ = true;
    }
    
    t_++;
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        // Update biased first moment estimate
        m_.data()[i] = beta1_ * m_.data()[i] + (1.0 - beta1_) * gradients.data()[i];
        
        // Update biased second raw moment estimate
        v_.data()[i] = beta2_ * v_.data()[i] + (1.0 - beta2_) * gradients.data()[i] * gradients.data()[i];
        
        // Compute bias-corrected first moment estimate
        double m_hat = m_.data()[i] / (1.0 - std::pow(beta1_, t_));
        
        // Compute bias-corrected second raw moment estimate
        double v_hat = v_.data()[i] / (1.0 - std::pow(beta2_, t_));
        
        // Update parameters
        parameters.data()[i] -= learning_rate_ * m_hat / (std::sqrt(v_hat) + epsilon_);
    }
}

void Adam::reset() {
    initialized_ = false;
    t_ = 0;
    m_ = Tensor();
    v_ = Tensor();
}

// RMSprop Implementation
RMSprop::RMSprop(double learning_rate, double decay, double epsilon)
    : learning_rate_(learning_rate), decay_(decay), epsilon_(epsilon), initialized_(false) {
    if (learning_rate <= 0.0) {
        throw std::invalid_argument("Learning rate must be positive");
    }
    if (decay < 0.0 || decay >= 1.0) {
        throw std::invalid_argument("Decay must be in [0, 1)");
    }
}

void RMSprop::step(Tensor& parameters, const Tensor& gradients) {
    if (parameters.size() != gradients.size()) {
        throw std::invalid_argument("Parameters and gradients must have the same size");
    }
    
    if (!initialized_) {
        cache_ = Tensor(parameters.shape(), 0.0);
        initialized_ = true;
    }
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        // Update cache: cache = decay * cache + (1 - decay) * gradient^2
        cache_.data()[i] = decay_ * cache_.data()[i] + 
                          (1.0 - decay_) * gradients.data()[i] * gradients.data()[i];
        
        // Update parameters
        parameters.data()[i] -= learning_rate_ * gradients.data()[i] / 
                               (std::sqrt(cache_.data()[i]) + epsilon_);
    }
}

void RMSprop::reset() {
    initialized_ = false;
    cache_ = Tensor();
}

} // namespace deep_learning
} // namespace ml
