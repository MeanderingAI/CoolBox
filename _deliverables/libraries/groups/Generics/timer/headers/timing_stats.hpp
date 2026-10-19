#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <ostream>

namespace generics::timer {

/// Point-in-time copy of the counters tracked by TimingStats /
/// ConcurrentTimingStats.
struct TimingSnapshot {
    std::uint64_t count = 0;            ///< Number of samples recorded.
    std::chrono::nanoseconds total{0};  ///< Sum of all recorded samples.
    std::chrono::nanoseconds min{0};    ///< Fastest recorded sample.
    std::chrono::nanoseconds max{0};    ///< Slowest recorded sample.

    /// Arithmetic mean of all recorded samples, in nanoseconds. 0 if no
    /// samples have been recorded yet.
    double mean_ns() const noexcept {
        return count == 0 ? 0.0
                          : static_cast<double>(total.count()) / static_cast<double>(count);
    }
};

inline std::ostream& operator<<(std::ostream& os, const TimingSnapshot& snap) {
    return os << "TimingStats{count=" << snap.count << ", total_ns=" << snap.total.count()
              << ", min_ns=" << snap.min.count() << ", max_ns=" << snap.max.count()
              << ", mean_ns=" << snap.mean_ns() << "}";
}

/// Single-threaded timing-sample aggregator (count/total/min/max/mean).
///
/// Not thread-safe - use ConcurrentTimingStats if more than one thread will
/// call record() on the same instance. Plain (non-atomic) members keep this
/// the lower-overhead option whenever all recording happens on one thread.
class TimingStats {
public:
    void record(std::chrono::nanoseconds sample) noexcept {
        if (count_ == 0 || sample < min_) {
            min_ = sample;
        }
        if (count_ == 0 || sample > max_) {
            max_ = sample;
        }
        total_ += sample;
        ++count_;
    }

    template <typename Rep, typename Period>
    void record(std::chrono::duration<Rep, Period> sample) noexcept {
        record(std::chrono::duration_cast<std::chrono::nanoseconds>(sample));
    }

    TimingSnapshot snapshot() const noexcept { return TimingSnapshot{count_, total_, min_, max_}; }

    void reset() noexcept {
        count_ = 0;
        total_ = std::chrono::nanoseconds::zero();
        min_ = std::chrono::nanoseconds::zero();
        max_ = std::chrono::nanoseconds::zero();
    }

    /// Prints the current snapshot, e.g. `std::cout << my_stats;`.
    friend std::ostream& operator<<(std::ostream& os, const TimingStats& stats) {
        return os << stats.snapshot();
    }

private:
    std::uint64_t count_ = 0;
    std::chrono::nanoseconds total_{0};
    std::chrono::nanoseconds min_{0};
    std::chrono::nanoseconds max_{0};
};

/// Thread-safe timing-sample aggregator: any number of threads may call
/// record() concurrently on the same instance with no external locking.
/// Same API/semantics as TimingStats (count/total/min/max/mean) - pick
/// this one whenever record() will be called from more than one thread,
/// e.g. a ScopedTimer inside each worker of a thread pool feeding one
/// shared stats object.
///
/// Implementation mirrors MemoryStats' peak-tracking: min/max use a
/// compare-exchange retry loop over plain atomics rather than a mutex, so
/// record() stays lock-free.
class ConcurrentTimingStats {
public:
    void record(std::chrono::nanoseconds sample) noexcept {
        count_.fetch_add(1, std::memory_order_relaxed);
        total_.fetch_add(sample.count(), std::memory_order_relaxed);

        std::int64_t observed_min = min_.load(std::memory_order_relaxed);
        while (sample.count() < observed_min &&
               !min_.compare_exchange_weak(observed_min, sample.count(), std::memory_order_relaxed)) {
            // observed_min is refreshed by compare_exchange_weak on failure; retry.
        }

        std::int64_t observed_max = max_.load(std::memory_order_relaxed);
        while (sample.count() > observed_max &&
               !max_.compare_exchange_weak(observed_max, sample.count(), std::memory_order_relaxed)) {
            // observed_max is refreshed by compare_exchange_weak on failure; retry.
        }
    }

    template <typename Rep, typename Period>
    void record(std::chrono::duration<Rep, Period> sample) noexcept {
        record(std::chrono::duration_cast<std::chrono::nanoseconds>(sample));
    }

    TimingSnapshot snapshot() const noexcept {
        const std::uint64_t count = count_.load(std::memory_order_relaxed);
        return TimingSnapshot{
            count,
            std::chrono::nanoseconds(total_.load(std::memory_order_relaxed)),
            std::chrono::nanoseconds(count == 0 ? 0 : min_.load(std::memory_order_relaxed)),
            std::chrono::nanoseconds(count == 0 ? 0 : max_.load(std::memory_order_relaxed)),
        };
    }

    void reset() noexcept {
        count_.store(0, std::memory_order_relaxed);
        total_.store(0, std::memory_order_relaxed);
        min_.store(std::numeric_limits<std::int64_t>::max(), std::memory_order_relaxed);
        max_.store(std::numeric_limits<std::int64_t>::min(), std::memory_order_relaxed);
    }

    /// Prints the current snapshot, e.g. `std::cout << my_stats;`.
    friend std::ostream& operator<<(std::ostream& os, const ConcurrentTimingStats& stats) {
        return os << stats.snapshot();
    }

private:
    std::atomic<std::uint64_t> count_{0};
    std::atomic<std::int64_t> total_{0};
    std::atomic<std::int64_t> min_{std::numeric_limits<std::int64_t>::max()};
    std::atomic<std::int64_t> max_{std::numeric_limits<std::int64_t>::min()};
};

} // namespace generics::timer
