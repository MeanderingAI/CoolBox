#include "tyst_framework.hpp"
#include "concurrent_linked_list.h"
#include <thread>
#include <vector>

using namespace data_structures;

TEST(ConcurrentLinkedListTest, PushFrontAndSize) {
    ConcurrentLinkedList<int> list;
    list.push_front(1);
    list.push_front(2);
    list.push_front(3);
    EXPECT_EQ(list.size(), 3u);
}

TEST(ConcurrentLinkedListTest, PushBack) {
    ConcurrentLinkedList<int> list;
    list.push_back(10);
    list.push_back(20);
    EXPECT_EQ(list.size(), 2u);
    EXPECT_TRUE(list.find(10));
    EXPECT_TRUE(list.find(20));
}

TEST(ConcurrentLinkedListTest, PopFront) {
    ConcurrentLinkedList<int> list;
    list.push_front(1);
    list.push_front(2);
    int val = 0;
    EXPECT_TRUE(list.pop_front(val));
    EXPECT_EQ(val, 2);
    EXPECT_EQ(list.size(), 1u);
}

TEST(ConcurrentLinkedListTest, RemoveValue) {
    ConcurrentLinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    EXPECT_TRUE(list.remove_value(2));
    EXPECT_FALSE(list.find(2));
    EXPECT_EQ(list.size(), 2u);
}

TEST(ConcurrentLinkedListTest, Find) {
    ConcurrentLinkedList<int> list;
    list.push_back(42);
    EXPECT_TRUE(list.find(42));
    EXPECT_FALSE(list.find(99));
}

TEST(ConcurrentLinkedListTest, EmptyList) {
    ConcurrentLinkedList<int> list;
    EXPECT_TRUE(list.empty());
    int val = 0;
    EXPECT_FALSE(list.pop_front(val));
}

TEST(ConcurrentLinkedListTest, ConcurrentPushFront) {
    ConcurrentLinkedList<int> list;
    const int num_threads = 4;
    const int inserts_per_thread = 100;
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&list, t, inserts_per_thread]() {
            for (int i = 0; i < inserts_per_thread; ++i) {
                list.push_front(t * inserts_per_thread + i);
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(list.size(), static_cast<size_t>(num_threads * inserts_per_thread));
}
