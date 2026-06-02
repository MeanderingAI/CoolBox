
#include "tyst_framework.hpp"
#include "thread_pool.h"
#include <atomic>
#include <chrono>
#include <thread>

TEST(ThreadPool, ExecutesTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        pool.enqueue([&counter] { counter.fetch_add(1); });
    }

    // Give workers a short time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.stop();

    EXPECT_EQ(counter.load(), 100);
}
