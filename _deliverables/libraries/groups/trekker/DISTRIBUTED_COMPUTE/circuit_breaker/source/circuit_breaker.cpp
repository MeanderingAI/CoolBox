#include "../headers/circuit_breaker.h"

namespace distributed {

CircuitBreaker::CircuitBreaker(CircuitBreakerConfig config)
    : config_(std::move(config)),
      state_(CircuitState::Closed),
      failure_count_(0),
      success_count_(0) {}

void CircuitBreaker::call(std::function<void()> fn) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == CircuitState::Open) {
            const auto elapsed =
                std::chrono::steady_clock::now() - opened_at_;
            if (elapsed >= config_.timeout) {
                transition(CircuitState::HalfOpen);
            } else {
                throw std::runtime_error(
                    "CircuitBreaker is OPEN — call rejected");
            }
        }
    }

    try {
        fn();
        record_success();
    } catch (...) {
        record_failure();
        throw;
    }
}

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_         = CircuitState::Closed;
    failure_count_ = 0;
    success_count_ = 0;
}

CircuitState CircuitBreaker::state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

std::uint32_t CircuitBreaker::failure_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return failure_count_;
}

std::uint32_t CircuitBreaker::success_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return success_count_;
}

std::string CircuitBreaker::state_name() const {
    switch (state()) {
        case CircuitState::Closed:   return "Closed";
        case CircuitState::Open:     return "Open";
        case CircuitState::HalfOpen: return "HalfOpen";
    }
    return "Unknown";
}

void CircuitBreaker::record_success() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++success_count_;
    failure_count_ = 0;
    if (state_ == CircuitState::HalfOpen &&
        success_count_ >= config_.success_threshold) {
        transition(CircuitState::Closed);
    }
}

void CircuitBreaker::record_failure() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++failure_count_;
    success_count_ = 0;
    if (state_ == CircuitState::Closed &&
        failure_count_ >= config_.failure_threshold) {
        transition(CircuitState::Open);
    } else if (state_ == CircuitState::HalfOpen) {
        transition(CircuitState::Open);
    }
}

void CircuitBreaker::transition(CircuitState next) {
    state_         = next;
    failure_count_ = 0;
    success_count_ = 0;
    if (next == CircuitState::Open) {
        opened_at_ = std::chrono::steady_clock::now();
    }
}

} // namespace distributed
