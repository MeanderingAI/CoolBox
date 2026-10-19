# CoolBox v3.1.1 Release Notes

## Python extensions for Generics timer

The `ml_toolbox` Python extension now exposes the Generics timer API through
`ml_toolbox.timer`. The extension uses the same monotonic C++ stopwatch and
statistics implementations introduced in v3.1.0.

### New Python API

- `timer.Timer` provides `start()`, `stop()`, `reset()`, `lap_ms()`,
  `elapsed_ms()`, and the read-only `is_running` property.
- `timer.TimingStats` records samples on one thread.
- `timer.ConcurrentTimingStats` safely accepts samples from multiple threads.
- `timer.TimingSnapshot` exposes read-only `count`, `total_ns`, `min_ns`,
  `max_ns`, and `mean_ns` values.
- `timer.ScopedTimer` reports elapsed milliseconds to a callback or records
  directly into either stats class. It supports `with` statements and an
  idempotent `close()` method.

Numeric units are explicit in the Python API: stopwatch results use
milliseconds and statistics samples and snapshots use nanoseconds.

### Stopwatch example

```python
import time

from ml_toolbox import timer

stopwatch = timer.Timer()
stopwatch.start()
time.sleep(0.01)
print(f"{stopwatch.elapsed_ms():.2f} ms")
stopwatch.stop()
```

### Scoped timing and statistics

```python
from ml_toolbox import timer

stats = timer.TimingStats()

with timer.ScopedTimer(stats):
    do_work()

snapshot = stats.snapshot()
print(snapshot.count, snapshot.mean_ns)
```

Use `ConcurrentTimingStats` instead when several Python or native worker
threads record into the same aggregator:

```python
from concurrent.futures import ThreadPoolExecutor

from ml_toolbox import timer

stats = timer.ConcurrentTimingStats()

def timed_work():
    with timer.ScopedTimer(stats):
        do_work()

with ThreadPoolExecutor() as executor:
    list(executor.map(lambda _: timed_work(), range(8)))
```

### Build integration and validation

The timer binding source and Generics timer headers are included by both the
CMake and setuptools extension builds. Python coverage verifies stopwatch
lifecycle behavior, statistics and reset semantics, concurrent recording,
context-manager reporting, idempotent close behavior, and callback reporting
during finalization.
