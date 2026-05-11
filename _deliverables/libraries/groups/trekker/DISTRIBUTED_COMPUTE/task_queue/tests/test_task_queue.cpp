#include "tyst_framework.hpp"

#include "task_queue.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

using namespace distributed;

TEST(TaskQueueTest, EnqueueAndDequeue) {
    TaskQueue<int> q;
    EXPECT_TRUE(q.enqueue(10));
    EXPECT_TRUE(q.enqueue(20));
    EXPECT_EQ(q.size(), 2u);

    auto v1 = q.dequeue();
    auto v2 = q.dequeue();
    EXPECT_TRUE(v1.has_value());
    EXPECT_TRUE(v2.has_value());
    EXPECT_EQ(*v1, 10);
    EXPECT_EQ(*v2, 20);
}

TEST(TaskQueueTest, TryDequeueOnEmptyQueue) {
    TaskQueue<int> q;
    auto v = q.try_dequeue();
    EXPECT_FALSE(v.has_value());
}

TEST(TaskQueueTest, CloseUnblocksBlockedConsumer) {
    TaskQueue<int> q;
    std::thread consumer([&] {
        auto v = q.dequeue();
        EXPECT_FALSE(v.has_value()); // returns nullopt on close
    });
    q.close();
    consumer.join();
    EXPECT_TRUE(q.is_closed());
}

TEST(TaskQueueTest, BoundedQueueRejectsBeyondCapacity) {
    TaskQueue<int> q(2);
    EXPECT_TRUE(q.try_enqueue(1));
    EXPECT_TRUE(q.try_enqueue(2));
    EXPECT_FALSE(q.try_enqueue(3)); // at capacity
}

TEST(TaskQueueTest, ConcurrentProducerConsumerSumsCorrectly) {
    TaskQueue<int> q;
    std::atomic<int> sum{0};

    std::thread producer([&] {
        for (int i = 1; i <= 100; ++i) q.enqueue(i);
        q.close();
    });
    std::thread consumer([&] {
        while (auto v = q.dequeue()) sum += *v;
    });
    producer.join();
    consumer.join();

    EXPECT_EQ(sum.load(), 5050);
}

TEST(TaskQueueTest, DequeueForTimesOutOnEmptyQueue) {
    TaskQueue<int> q;
    auto v = q.dequeue_for(std::chrono::milliseconds(20));
    EXPECT_FALSE(v.has_value());
}

TEST(TaskQueueTest, FunctionTasksExecute) {
    TaskQueue<std::function<void()>> q;
    std::atomic<int> counter{0};

    q.enqueue([&] { ++counter; });
    q.enqueue([&] { ++counter; });

    while (auto task = q.try_dequeue()) (*task)();
    EXPECT_EQ(counter.load(), 2);
}
