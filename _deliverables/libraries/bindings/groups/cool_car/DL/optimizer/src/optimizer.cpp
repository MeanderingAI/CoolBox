#include "optimizer.h"
#include <cmath>

namespace ml {
namespace deep_learning {

// SGD
SGD::SGD(double learning_rate, double momentum)
    : learning_rate_(learning_rate), momentum_(momentum), initialized_(false) {}

void SGD::step(Tensor& parameters, const Tensor& gradients) {
    if (!initialized_) {
        velocity_ = Tensor(parameters.shape(), 0.0);
        initialized_ = true;
    }
    for (size_t i = 0; i < parameters.size(); ++i) {
        velocity_.data()[i] = momentum_ * velocity_.data()[i] - learning_rate_ * gradients.data()[i];
        parameters.data()[i] += velocity_.data()[i];
    }
}

void SGD::reset() {
    initialized_ = false;
    velocity_ = Tensor();
}

// Adam
Adam::Adam(double learning_rate, double beta1, double beta2, double epsilon)
    : learning_rate_(learning_rate), beta1_(beta1), beta2_(beta2),
      epsilon_(epsilon), t_(0), initialized_(false) {}

void Adam::step(Tensor& parameters, const Tensor& gradients) {
    if (!initialized_) {
        m_ = Tensor(parameters.shape(), 0.0);
        v_ = Tensor(parameters.shape(), 0.0);
        initialized_ = true;
    }
    t_++;
    for (size_t i = 0; i < parameters.size(); ++i) {
        m_.data()[i] = beta1_ * m_.data()[i] + (1.0 - beta1_) * gradients.data()[i];
        v_.data()[i] = beta2_ * v_.data()[i] + (1.0 - beta2_) * gradients.data()[i] * gradients.data()[i];
        double m_hat = m_.data()[i] / (1.0 - std::pow(beta1_, t_));
        double v_hat = v_.data()[i] / (1.0 - std::pow(beta2_, t_));
        parameters.data()[i] -= learning_rate_ * m_hat / (std::sqrt(v_hat) + epsilon_);
    }
}

void Adam::reset() {
    initialized_ = false;
    t_ = 0;
    m_ = Tensor();
    v_ = Tensor();
}

// RMSprop
RMSprop::RMSprop(double learning_rate, double decay, double epsilon)
    : learning_rate_(learning_rate), decay_(decay), epsilon_(epsilon), initialized_(false) {}

void RMSprop::step(Tensor& parameters, const Tensor& gradients) {
    if (!initialized_) {
        cache_ = Tensor(parameters.shape(), 0.0);
        initialized_ = true;
    }
    for (size_t i = 0; i < parameters.size(); ++i) {
        cache_.data()[i] = decay_ * cache_.data()[i] + (1.0 - decay_) * gradients.data()[i] * gradients.data()[i];
        parameters.data()[i] -= learning_rate_ * gradients.data()[i] / (std::sqrt(cache_.data()[i]) + epsilon_);
    }
}

void RMSprop::reset() {
    initialized_ = false;
    cache_ = Tensor();
}

} // namespace deep_learning
} // namespace ml
