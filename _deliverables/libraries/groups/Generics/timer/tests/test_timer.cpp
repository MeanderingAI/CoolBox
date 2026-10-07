/**
 * @file test_timer.cpp
 * @brief Test suite for the Generics timer module.
 *
 * Tests cover:
 *   - Timer: start/stop/reset/lap/elapsed, single-threaded stopwatch
 *   - TimingStats: single-threaded count/total/min/max/mean aggregation
 *   - ConcurrentTimingStats: thread-safe aggregation under real concurrent
 *     record() calls from multiple std::thread workers
 *   - ScopedTimer: RAII scope timing, print_to() and record_into() callbacks
 */

#include <tyst_framework.hpp>

#include "generics_timer.hpp"

#include <atomic>
#include <chrono>
#include <sstream>
#include <thread>
#include <vector>

using namespace generics::timer;
using namespace std::chrono_literals;

// ===================================================================
// Timer tests
// ===================================================================

TYST_TEST(TimerTest, StartsNotRunningByDefault) {
    Timer t;
    TYST_EXPECT_FALSE(t.is_running());
    TYST_EXPECT_EQ(t.elapsed<std::chrono::nanoseconds>().count(), 0);
}

TYST_TEST(TimerTest, MeasuresElapsedTimeWhileRunning) {
    Timer t;
    t.start();
    std::this_thread::sleep_for(10ms);
    const auto elapsed = t.elapsed<std::chrono::duration<double, std::milli>>();
    TYST_EXPECT_TRUE(t.is_running());
    TYST_EXPECT_GE(elapsed.count(), 5.0); // generous lower bound to avoid flakiness
}

TYST_TEST(TimerTest, StopFreezesElapsedTime) {
    Timer t;
    t.start();
    std::this_thread::sleep_for(10ms);
    t.stop();
    TYST_EXPECT_FALSE(t.is_running());
    const auto first_read = t.elapsed<std::chrono::nanoseconds>().count();
    std::this_thread::sleep_for(10ms);
    const auto second_read = t.elapsed<std::chrono::nanoseconds>().count();
    TYST_EXPECT_EQ(first_read, second_read);
}

TYST_TEST(TimerTest, ResetClearsAccumulatedTime) {
    Timer t;
    t.start();
    std::this_thread::sleep_for(10ms);
    t.stop();
    t.reset();
    TYST_EXPECT_FALSE(t.is_running());
    TYST_EXPECT_EQ(t.elapsed<std::chrono::nanoseconds>().count(), 0);
}

TYST_TEST(TimerTest, LapReturnsElapsedAndRestarts) {
    Timer t;
    t.start();
    std::this_thread::sleep_for(10ms);
    const auto lap1 = t.lap<std::chrono::duration<double, std::milli>>();
    TYST_EXPECT_GE(lap1.count(), 5.0);
    TYST_EXPECT_TRUE(t.is_running());

    std::this_thread::sleep_for(10ms);
    const auto lap2 = t.lap<std::chrono::duration<double, std::milli>>();
    TYST_EXPECT_GE(lap2.count(), 5.0);
}

// ===================================================================
// TimingStats tests (single-threaded)
// ===================================================================

TYST_TEST(TimingStatsTest, StartsEmpty) {
    TimingStats stats;
    const auto snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.count, 0u);
    TYST_EXPECT_EQ(snap.total.count(), 0);
    TYST_EXPECT_DOUBLE_EQ(snap.mean_ns(), 0.0);
}

TYST_TEST(TimingStatsTest, TracksCountTotalMinMaxMean) {
    TimingStats stats;
    stats.record(std::chrono::nanoseconds(100));
    stats.record(std::chrono::nanoseconds(300));
    stats.record(std::chrono::nanoseconds(200));

    const auto snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.count, 3u);
    TYST_EXPECT_EQ(snap.total.count(), 600);
    TYST_EXPECT_EQ(snap.min.count(), 100);
    TYST_EXPECT_EQ(snap.max.count(), 300);
    TYST_EXPECT_DOUBLE_EQ(snap.mean_ns(), 200.0);
}

TYST_TEST(TimingStatsTest, AcceptsAnyDurationType) {
    TimingStats stats;
    stats.record(std::chrono::microseconds(1)); // 1000ns
    stats.record(std::chrono::milliseconds(1)); // 1,000,000ns

    const auto snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.count, 2u);
    TYST_EXPECT_EQ(snap.total.count(), 1'001'000);
}

TYST_TEST(TimingStatsTest, ResetClearsCounters) {
    TimingStats stats;
    stats.record(std::chrono::nanoseconds(100));
    stats.reset();
    const auto snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.count, 0u);
    TYST_EXPECT_EQ(snap.total.count(), 0);
}

TYST_TEST(TimingStatsTest, StreamOperatorPrintsSnapshot) {
    TimingStats stats;
    stats.record(std::chrono::nanoseconds(500));
    std::ostringstream oss;
    oss << stats;
    TYST_EXPECT_NE(oss.str().find("count=1"), std::string::npos);
    TYST_EXPECT_NE(oss.str().find("total_ns=500"), std::string::npos);
}

// ===================================================================
// ConcurrentTimingStats tests (multi-threaded)
// ===================================================================

TYST_TEST(ConcurrentTimingStatsTest, StartsEmpty) {
    ConcurrentTimingStats stats;
    const auto snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.count, 0u);
    TYST_EXPECT_DOUBLE_EQ(snap.mean_ns(), 0.0);
}

TYST_TEST(ConcurrentTimingStatsTest, TracksCountTotalMinMaxSingleThreaded) {
    ConcurrentTimingStats stats;
    stats.record(std::chrono::nanoseconds(100));
    stats.record(std::chrono::nanoseconds(300));
    stats.record(std::chrono::nanoseconds(200));

    const auto snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.count, 3u);
    TYST_EXPECT_EQ(snap.total.count(), 600);
    TYST_EXPECT_EQ(snap.min.count(), 100);
    TYST_EXPECT_EQ(snap.max.count(), 300);
}

TYST_TEST(ConcurrentTimingStatsTest, AggregatesConcurrentRecordsFromManyThreads) {
    ConcurrentTimingStats stats;
    constexpr int kThreads = 8;
    constexpr int kSamplesPerThread = 1000;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&stats, t, kSamplesPerThread]() {
            for (int i = 0; i < kSamplesPerThread; ++i) {
                // Distinct values per thread so min/max are easy to predict,
                // while all 8 threads hammer the same shared stats object.
                stats.record(std::chrono::nanoseconds(t * kSamplesPerThread + i + 1));
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    const auto snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.count, static_cast<std::uint64_t>(kThreads * kSamplesPerThread));
    TYST_EXPECT_EQ(snap.min.count(), 1);
    TYST_EXPECT_EQ(snap.max.count(), kThreads * kSamplesPerThread);

    // Sum of 1..N = N*(N+1)/2.
    const long long n = kThreads * kSamplesPerThread;
    TYST_EXPECT_EQ(snap.total.count(), n * (n + 1) / 2);
}

TYST_TEST(ConcurrentTimingStatsTest, ResetClearsCountersAndMinMaxSentinels) {
    ConcurrentTimingStats stats;
    stats.record(std::chrono::nanoseconds(500));
    stats.reset();

    const auto snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.count, 0u);
    TYST_EXPECT_EQ(snap.min.count(), 0); // reported as 0 when empty, not the sentinel
    TYST_EXPECT_EQ(snap.max.count(), 0);

    // Still usable after reset.
    stats.record(std::chrono::nanoseconds(42));
    const auto snap2 = stats.snapshot();
    TYST_EXPECT_EQ(snap2.count, 1u);
    TYST_EXPECT_EQ(snap2.min.count(), 42);
}

// ===================================================================
// ScopedTimer tests
// ===================================================================

TYST_TEST(ScopedTimerTest, InvokesCallbackOnScopeExit) {
    bool called = false;
    double reported_ms = -1.0;
    {
        ScopedTimer t([&](std::chrono::duration<double, std::milli> elapsed) {
            called = true;
            reported_ms = elapsed.count();
        });
        std::this_thread::sleep_for(10ms);
        TYST_EXPECT_FALSE(called); // not yet - still inside the scope
    }
    TYST_EXPECT_TRUE(called);
    TYST_EXPECT_GE(reported_ms, 5.0);
}

TYST_TEST(ScopedTimerTest, PrintToWritesLabelAndElapsed) {
    std::ostringstream oss;
    {
        ScopedTimer t(print_to(oss, "my_block"));
        std::this_thread::sleep_for(1ms);
    }
    const std::string output = oss.str();
    TYST_EXPECT_NE(output.find("my_block:"), std::string::npos);
    TYST_EXPECT_NE(output.find("ms"), std::string::npos);
}

TYST_TEST(ScopedTimerTest, RecordIntoFeedsTimingStats) {
    TimingStats stats;
    {
        ScopedTimer t(record_into(stats));
        std::this_thread::sleep_for(1ms);
    }
    TYST_EXPECT_EQ(stats.snapshot().count, 1u);
    TYST_EXPECT_GT(stats.snapshot().total.count(), 0);
}

TYST_TEST(ScopedTimerTest, RecordIntoFeedsConcurrentTimingStatsFromMultipleThreads) {
    ConcurrentTimingStats stats;
    constexpr int kThreads = 4;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&stats]() {
            ScopedTimer timer(record_into(stats));
            std::this_thread::sleep_for(1ms);
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    TYST_EXPECT_EQ(stats.snapshot().count, static_cast<std::uint64_t>(kThreads));
}
