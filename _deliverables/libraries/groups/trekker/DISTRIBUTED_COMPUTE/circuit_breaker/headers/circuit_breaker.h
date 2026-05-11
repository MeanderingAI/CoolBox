#ifndef DISTRIBUTED_CIRCUIT_BREAKER_H
#define DISTRIBUTED_CIRCUIT_BREAKER_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>

namespace distributed {

// Circuit breaker states: Closed (normal) → Open (fast-fail) → HalfOpen (probe)
enum class CircuitState { Closed, Open, HalfOpen };

struct CircuitBreakerConfig {
    std::uint32_t             failure_threshold = 5;    // failures before opening
    std::uint32_t             success_threshold = 2;    // successes in HalfOpen to close
    std::chrono::milliseconds timeout{10000};            // wait before probing (HalfOpen)
};

// Circuit breaker wrapping a callable. When failures exceed the threshold the
// circuit opens and subsequent calls throw std::runtime_error immediately.
// After `timeout` the circuit enters HalfOpen to allow a probe; on enough
// consecutive successes it transitions back to Closed.
class CircuitBreaker {
public:
    explicit CircuitBreaker(CircuitBreakerConfig config = {});

    // Execute fn. Throws std::runtime_error when the circuit is Open.
    // Any exception thrown by fn counts as a failure.
    void call(std::function<void()> fn);

    // Manually reset to Closed state.
    void reset();

    CircuitState  state()         const;
    std::uint32_t failure_count() const;
    std::uint32_t success_count() const;
    std::string   state_name()    const;

private:
    void record_success();
    void record_failure();
    void transition(CircuitState next); // must be called under lock

    mutable std::mutex                    mutex_;
    CircuitBreakerConfig                  config_;
    CircuitState                          state_;
    std::uint32_t                         failure_count_;
    std::uint32_t                         success_count_;
    std::chrono::steady_clock::time_point opened_at_;
};

} // namespace distributed

#endif // DISTRIBUTED_CIRCUIT_BREAKER_H
