#pragma once

/// Umbrella header for the Generics timer module.
///
/// - Timer: single-threaded, zero-overhead stopwatch (start/stop/lap/elapsed).
/// - TimingStats: single-threaded count/total/min/max/mean aggregator.
/// - ConcurrentTimingStats: thread-safe (lock-free) version of the same
///   aggregator, for recording samples produced by multiple threads.
/// - ScopedTimer: RAII helper that times a scope and reports the elapsed
///   time via a callback (print it, or feed it into either stats type).
#include "scoped_timer.hpp"
#include "timer.hpp"
#include "timing_stats.hpp"
