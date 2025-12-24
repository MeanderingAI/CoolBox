#include <gtest/gtest.h>
#include "data_structures/concurrent_linked_list.h"
#include <thread>
#include <vector>

using namespace data_structures;

TEST(ConcurrentLinkedListTest, PushFront) {
    ConcurrentLinkedList<int> list;
    
    list.push_front(3);
    list.push_front(2);
    list.push_front(1);
    
    EXPECT_EQ(list.size(), 3);
    EXPECT_FALSE(list.empty());
}

TEST(ConcurrentLinkedListTest, PushBack) {
    ConcurrentLinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_EQ(list.size(), 3);
    EXPECT_FALSE(list.empty());
}

TEST(ConcurrentLinkedListTest, PopFront) {
    ConcurrentLinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    int value;
    EXPECT_TRUE(list.pop_front(value));
    EXPECT_EQ(value, 1);
    EXPECT_EQ(list.size(), 2);
    
    EXPECT_TRUE(list.pop_front(value));
    EXPECT_EQ(value, 2);
    
    EXPECT_TRUE(list.pop_front(value));
    EXPECT_EQ(value, 3);
    
    EXPECT_FALSE(list.pop_front(value));
    EXPECT_TRUE(list.empty());
}

TEST(ConcurrentLinkedListTest, Find) {
    ConcurrentLinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_TRUE(list.find(2));
    EXPECT_FALSE(list.find(10));
}

TEST(ConcurrentLinkedListTest, RemoveValue) {
    ConcurrentLinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_TRUE(list.remove_value(2));
    EXPECT_EQ(list.size(), 2);
    EXPECT_FALSE(list.find(2));
    
    EXPECT_FALSE(list.remove_value(10));
    EXPECT_EQ(list.size(), 2);
}

TEST(ConcurrentLinkedListTest, ConcurrentPushFront) {
    ConcurrentLinkedList<int> list;
    const int num_threads = 4;
    const int items_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&list, t, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                list.push_front(t * items_per_thread + i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(list.size(), num_threads * items_per_thread);
}

TEST(ConcurrentLinkedListTest, ConcurrentPushBack) {
    ConcurrentLinkedList<int> list;
    const int num_threads = 4;
    const int items_per_thread = 50;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&list, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                list.push_back(i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(list.size(), num_threads * items_per_thread);
}

TEST(ConcurrentLinkedListTest, ConcurrentPopFront) {
    ConcurrentLinkedList<int> list;
    
    // Pre-populate list
    for (int i = 0; i < 100; ++i) {
        list.push_back(i);
    }
    
    std::vector<std::thread> threads;
    std::vector<int> popped_values[4];
    
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&list, &popped_values, t]() {
            int value;
            while (list.pop_front(value)) {
                popped_values[t].push_back(value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_TRUE(list.empty());
    
    // Count total popped values
    int total_popped = 0;
    for (int t = 0; t < 4; ++t) {
        total_popped += popped_values[t].size();
    }
    EXPECT_EQ(total_popped, 100);
}

TEST(ConcurrentLinkedListTest, ProducerConsumer) {
    ConcurrentLinkedList<int> list;
    const int num_items = 1000;
    
    // Producer thread
    std::thread producer([&list, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            list.push_back(i);
        }
    });
    
    // Consumer thread
    int consumed = 0;
    std::thread consumer([&list, &consumed, num_items]() {
        int value;
        while (consumed < num_items) {
            if (list.pop_front(value)) {
                consumed++;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(consumed, num_items);
}

TEST(ConcurrentLinkedListTest, ConcurrentFindAndRemove) {
    ConcurrentLinkedList<int> list;
    
    // Pre-populate
    for (int i = 0; i < 100; ++i) {
        list.push_back(i);
    }
    
    std::vector<std::thread> threads;
    
    // Reader threads
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&list]() {
            for (int i = 0; i < 100; ++i) {
                list.find(i);
            }
        });
    }
    
    // Remover threads
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&list, t]() {
            for (int i = t; i < 100; i += 2) {
                list.remove_value(i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All items should be removed
    EXPECT_EQ(list.size(), 0);
}
