
#include "tyst_framework.hpp"
#include "folio_queue.h"
#include <thread>
#include <vector>

using namespace tyst::framework;

TEST(FolioQueueTest, SingleThread) {
    folio_queue<int> fq;
    fq.enqueue(1);
    fq.enqueue(2);
    fq.enqueue(3);
    EXPECT_EQ(fq.size(), 3u);
    EXPECT_EQ(fq.dequeue(), 1);
    EXPECT_EQ(fq.dequeue(), 2);
    EXPECT_EQ(fq.dequeue(), 3);
    EXPECT_TRUE(fq.empty());
}

TEST(FolioQueueTest, Batch) {
    folio_queue<int> fq;
    fq.enqueue_batch({10, 20, 30, 40});
    auto folio = fq.dequeue_folio(3);
    EXPECT_EQ(folio.size(), 3u);
    EXPECT_EQ(folio[0], 10);
    EXPECT_EQ(folio[1], 20);
    EXPECT_EQ(folio[2], 30);
    EXPECT_EQ(fq.size(), 1u);
    EXPECT_EQ(fq.dequeue(), 40);
}

TEST(FolioQueueTest, MultiThread) {
    folio_queue<int> fq;
    std::thread producer([&](){
        for (int i = 0; i < 5; ++i) fq.enqueue(i);
    });
    std::vector<int> results;
    std::thread consumer([&](){
        for (int i = 0; i < 5; ++i) results.push_back(fq.dequeue());
    });
    producer.join();
    consumer.join();
    for (int i = 0; i < 5; ++i) EXPECT_EQ(results[i], i);
}

// No main() needed, tyst_framework provides it.
