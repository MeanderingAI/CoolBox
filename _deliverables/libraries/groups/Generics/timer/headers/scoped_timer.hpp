#pragma once

#include <chrono>
#include <functional>
#include <ostream>
#include <string>
#include <utility>

#include "timer.hpp"

namespace generics::timer {

/// RAII stopwatch that times the enclosing scope and reports the elapsed
/// duration when it is destroyed - i.e. when the scope ends, including via
/// an exception. Construct one at the top of a function/block you want to
/// measure the execution speed of; there's no manual start()/stop() to
/// forget.
///
/// ScopedTimer has no opinion on *where* the measurement goes - the
/// destination is a callback supplied at construction. Use print_to() to
/// log it, or record_into() to feed a TimingStats (single-threaded) or
/// ConcurrentTimingStats (multi-threaded) aggregator instead:
///
/// \code
///   { // single-threaded function body
///     ScopedTimer t(print_to(std::cout, "do_work"));
///     do_work();
///   } // "do_work: 12.3ms" printed here
/// \endcode
class ScopedTimer {
public:
    using Callback = std::function<void(std::chrono::duration<double, std::milli>)>;

    explicit ScopedTimer(Callback on_stop) : on_stop_(std::move(on_stop)) { timer_.start(); }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

    ~ScopedTimer() {
        if (on_stop_) {
            on_stop_(timer_.elapsed<std::chrono::duration<double, std::milli>>());
        }
    }

    /// Elapsed time so far, without waiting for destruction.
    template <typename Duration = std::chrono::duration<double, std::milli>>
    Duration elapsed() const noexcept {
        return timer_.elapsed<Duration>();
    }

private:
    Timer timer_;
    Callback on_stop_;
};

/// Builds a ScopedTimer callback that feeds a nanosecond sample into any
/// stats object exposing a `record(std::chrono::nanoseconds)` member - e.g.
/// TimingStats or ConcurrentTimingStats - with no inheritance required
/// of Stats; only the member function signature needs to match. Pass a
/// ConcurrentTimingStats when the same `stats` instance is shared across
/// threads (e.g. one ScopedTimer per task inside a thread pool, all
/// recording into one shared aggregator); pass a TimingStats when a single
/// thread owns `stats`.
template <typename Stats>
ScopedTimer::Callback record_into(Stats& stats) {
    return [&stats](std::chrono::duration<double, std::milli> elapsed) {
        stats.record(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed));
    };
}

/// Builds a ScopedTimer callback that writes "<label>: <elapsed>ms\n" to os.
inline ScopedTimer::Callback print_to(std::ostream& os, std::string label) {
    return [&os, label = std::move(label)](std::chrono::duration<double, std::milli> elapsed) {
        os << label << ": " << elapsed.count() << "ms\n";
    };
}

} // namespace generics::timer
