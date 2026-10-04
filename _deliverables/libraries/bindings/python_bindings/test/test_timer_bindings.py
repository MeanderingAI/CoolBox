import gc
import threading
import time

from ml_toolbox import timer


def test_timer_start_stop_reset_and_lap():
    stopwatch = timer.Timer()
    stopwatch.start()
    time.sleep(0.002)

    assert stopwatch.is_running
    assert stopwatch.elapsed_ms() > 0
    assert stopwatch.lap_ms() > 0
    assert stopwatch.is_running

    stopwatch.stop()
    stopped_ms = stopwatch.elapsed_ms()
    time.sleep(0.001)
    assert stopwatch.elapsed_ms() == stopped_ms

    stopwatch.reset()
    assert not stopwatch.is_running
    assert stopwatch.elapsed_ms() == 0


def test_timing_stats_snapshot_and_reset():
    stats = timer.TimingStats()
    stats.record_ns(100)
    stats.record_ns(300)
    stats.record_ns(200)

    snapshot = stats.snapshot()
    assert snapshot.count == 3
    assert snapshot.total_ns == 600
    assert snapshot.min_ns == 100
    assert snapshot.max_ns == 300
    assert snapshot.mean_ns == 200

    stats.reset()
    assert stats.snapshot().count == 0


def test_concurrent_timing_stats_accepts_samples_from_threads():
    stats = timer.ConcurrentTimingStats()
    workers = [
        threading.Thread(target=stats.record_ns, args=(sample,))
        for sample in range(1, 101)
    ]

    for worker in workers:
        worker.start()
    for worker in workers:
        worker.join()

    snapshot = stats.snapshot()
    assert snapshot.count == 100
    assert snapshot.total_ns == 5050
    assert snapshot.min_ns == 1
    assert snapshot.max_ns == 100


def test_scoped_timer_context_manager_records_once():
    stats = timer.TimingStats()

    with timer.ScopedTimer(stats) as scoped:
        time.sleep(0.001)
        assert scoped.elapsed_ms() > 0

    assert stats.snapshot().count == 1
    scoped.close()
    assert stats.snapshot().count == 1


def test_scoped_timer_callback_runs_on_finalization():
    samples = []
    scoped = timer.ScopedTimer(samples.append)
    del scoped
    gc.collect()

    assert len(samples) == 1
    assert samples[0] >= 0
