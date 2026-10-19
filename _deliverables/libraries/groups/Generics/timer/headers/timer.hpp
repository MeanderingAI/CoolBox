#pragma once

#include <chrono>

namespace generics::timer {

/// Single-threaded, monotonic stopwatch for measuring elapsed wall-clock
/// time (e.g. "how long did this call take"). Not thread-safe by design:
/// a single Timer instance must not be started/stopped/read from more than
/// one thread concurrently. Each thread that wants to time its own work
/// should own its own Timer - that keeps the common case (one thread,
/// start/stop around a call) free of any locking or atomic overhead.
///
/// To safely aggregate timing samples produced by multiple threads, record
/// each Timer's result into a ConcurrentTimingStats (see timing_stats.hpp).
class Timer {
public:
    using clock = std::chrono::steady_clock;

    /// Starts (or restarts) the timer running from now. Any previously
    /// accumulated time (from a prior start()/stop() pair) is kept; call
    /// reset() first to time from zero.
    void start() noexcept {
        start_ = clock::now();
        running_ = true;
    }

    /// Stops the timer, freezing elapsed() at the time of this call.
    /// No-op if the timer isn't currently running.
    void stop() noexcept {
        if (running_) {
            accumulated_ += clock::now() - start_;
            running_ = false;
        }
    }

    /// Stops the timer and clears it back to zero elapsed time.
    void reset() noexcept {
        accumulated_ = clock::duration::zero();
        running_ = false;
    }

    /// Stops the timer, returns the elapsed time since the previous
    /// start(), then immediately starts it again from zero. Handy for
    /// timing a sequence of steps with a single Timer instance, e.g.
    /// `auto step1 = t.lap(); auto step2 = t.lap();`.
    template <typename Duration = std::chrono::duration<double, std::milli>>
    Duration lap() noexcept {
        const Duration elapsed_time = elapsed<Duration>();
        reset();
        start();
        return elapsed_time;
    }

    bool is_running() const noexcept { return running_; }

    /// Total elapsed time, converted to Duration (default: milliseconds as
    /// a double). Safe to call while running - it reflects time elapsed
    /// "so far" without stopping the timer.
    template <typename Duration = std::chrono::duration<double, std::milli>>
    Duration elapsed() const noexcept {
        clock::duration total = accumulated_;
        if (running_) {
            total += clock::now() - start_;
        }
        return std::chrono::duration_cast<Duration>(total);
    }

private:
    clock::time_point start_{};
    clock::duration accumulated_{clock::duration::zero()};
    bool running_ = false;
};

} // namespace generics::timer
