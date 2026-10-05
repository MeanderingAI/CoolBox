Adding a stopwatch timer and single-/multi-threaded timing stats to Generics

## Summary

Added a `timer` module to `Generics` providing a single-threaded stopwatch
(`Timer`), two interchangeable timing-sample aggregators — one
single-threaded (`TimingStats`) and one thread-safe (`ConcurrentTimingStats`)
— and a `ScopedTimer` RAII helper that times a scope and reports the result
via a callback, without needing to manually start/stop anything.

## New module (`_deliverables/libraries/groups/Generics/timer/`)

- `generics::timer::Timer` — a monotonic (`std::chrono::steady_clock`)
  stopwatch: `start()`/`stop()`/`reset()`/`lap<Duration>()`/
  `elapsed<Duration>()`. Deliberately not thread-safe — plain (non-atomic)
  members keep the common case (one thread timing its own work) free of any
  synchronization overhead. Each thread that wants to time its own work
  should own its own `Timer`.
- `generics::timer::TimingStats` — single-threaded aggregator tracking
  count/total/min/max/mean over any number of recorded samples, via
  `record(duration)`. Plain members, no locking.
- `generics::timer::ConcurrentTimingStats` — the thread-safe counterpart:
  identical API/semantics, but any number of threads may call `record()` on
  the same instance concurrently with no external locking. Count/total use
  `fetch_add`; min/max use the same lock-free compare-exchange retry loop
  that `MemoryStats` uses for peak-byte tracking.
- `generics::timer::ScopedTimer` — RAII stopwatch: starts timing on
  construction, reports the elapsed time via a callback on destruction (i.e.
  when the scope ends, including via an exception). `print_to(os, label)`
  builds a callback that logs the result; `record_into(stats)` builds one
  that feeds either `TimingStats` or `ConcurrentTimingStats` — whichever is
  passed in, with no inheritance required of either.
- `generics_timer_tests` (tyst framework, 18 tests) covering `Timer`
  start/stop/reset/lap/elapsed semantics, `TimingStats` aggregation and
  reset, `ConcurrentTimingStats` aggregation under real concurrent
  `record()` calls from multiple `std::thread` workers (verified additionally
  under ThreadSanitizer with zero reported data races), and `ScopedTimer`'s
  scope-exit callback, `print_to`, and `record_into` (including from
  multiple threads sharing one `ConcurrentTimingStats`).

## Why two timing-stats variants?

`TimingStats` and `ConcurrentTimingStats` expose the exact same interface —
`record(duration)`, `snapshot()`, `reset()`, `operator<<` — so code written
against one works unchanged against the other (this is what lets
`ScopedTimer`'s `record_into()` accept either with no inheritance or
interface requirement, just a matching member function). They differ only
in how that shared interface is implemented internally:

```cpp
// TimingStats: plain members, single-threaded only
std::uint64_t count_ = 0;
std::chrono::nanoseconds total_{0};

// ConcurrentTimingStats: atomics, safe from any number of threads
std::atomic<std::uint64_t> count_{0};
std::atomic<std::int64_t> total_{0};
```

### Comparison

| | `TimingStats` | `ConcurrentTimingStats` |
|---|---|---|
| Safe to `record()` from multiple threads at once | No | Yes (lock-free) |
| Internal storage | Plain `std::uint64_t`/`std::chrono::nanoseconds` | `std::atomic<...>` |
| Overhead per `record()` call | A few plain arithmetic ops | A few atomic RMW ops + CAS retry loop for min/max |
| Typical use | One thread owns the aggregator (e.g. timing steps in a single request handler) | One aggregator shared across a thread pool / parallel workers |
| `Timer` (the stopwatch itself) | Still single-threaded either way — only the *aggregator* that samples feed into needs the thread-safe variant | |

Why not make `TimingStats` atomic always and drop the single-threaded
variant? Atomics and the min/max compare-exchange retry loop aren't free —
charging every single-threaded caller (the common case: timing one
function in one thread) for synchronization they don't need would be
wasteful, so, as with `MemoryStats`' optional global `operator new`/`delete`
override, the costlier thread-safe behavior is something you opt into by
naming `ConcurrentTimingStats` explicitly, not something paid for
unconditionally.

Why not make `Timer` itself thread-safe instead of adding a second stats
type? Because the concurrency problem in practice isn't "multiple threads
racing to start/stop *the same* stopwatch" — each thread should just own its
own `Timer` — it's "multiple threads each producing a timing sample that all
need to land in one shared aggregate". Solving that at the aggregator layer
(`ConcurrentTimingStats`) keeps `Timer` itself simple and zero-overhead,
while still giving `ScopedTimer` a clean way to fan results from many
threads into one place.

## Example usage

### `Timer`

```cpp
#include "timer.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using namespace generics::timer;
using namespace std::chrono_literals;

void example() {
    Timer t;
    t.start();
    std::this_thread::sleep_for(10ms);
    std::cout << t.elapsed<std::chrono::duration<double, std::milli>>().count() << "ms\n";
    // -> something like "10.08ms" (wall-clock, so not bit-for-bit exact)
    t.stop(); // freezes elapsed() at its current value
}
```

### `TimingStats` (single-threaded)

```cpp
#include "timing_stats.hpp"
#include <iostream>

using namespace generics::timer;

void example() {
    TimingStats stats;
    stats.record(std::chrono::nanoseconds(100));
    stats.record(std::chrono::nanoseconds(300));
    stats.record(std::chrono::nanoseconds(200));

    std::cout << stats << '\n';
    // -> TimingStats{count=3, total_ns=600, min_ns=100, max_ns=300, mean_ns=200}
}
```

### `ConcurrentTimingStats` (multi-threaded)

```cpp
#include "timing_stats.hpp"
#include <iostream>
#include <thread>
#include <vector>

using namespace generics::timer;

void example() {
    ConcurrentTimingStats stats; // one aggregator, shared across threads
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&stats, i]() {
            stats.record(std::chrono::nanoseconds((i + 1) * 100));
        });
    }
    for (auto& w : workers) {
        w.join();
    }

    std::cout << stats << '\n';
    // -> TimingStats{count=4, total_ns=1000, min_ns=100, max_ns=400, mean_ns=250}
}
```

### `ScopedTimer`

```cpp
#include "scoped_timer.hpp"
#include "timing_stats.hpp"
#include <iostream>

using namespace generics::timer;

void do_work();

void example() {
    TimingStats stats;
    {
        ScopedTimer t(record_into(stats));
        do_work();
    } // elapsed time recorded into stats here
    std::cout << "samples recorded: " << stats.snapshot().count << '\n';
    // -> samples recorded: 1

    {
        ScopedTimer t(print_to(std::cout, "do_work"));
        do_work();
    } // prints "do_work: <elapsed>ms" here, e.g. "do_work: 5.07ms"
}
```

`record_into(stats)` works identically whether `stats` is a `TimingStats`
(single thread) or a `ConcurrentTimingStats` (shared across a thread pool,
one `ScopedTimer` per task) — pick whichever matches how many threads will
be recording into that particular aggregator.
