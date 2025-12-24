#include <gtest/gtest.h>
#include "data_structures/concurrent_hash_map.h"
#include <thread>
#include <vector>
#include <algorithm>

using namespace data_structures;

TEST(ConcurrentHashMapTest, InsertAndGet) {
    ConcurrentHashMap<std::string, int> map;
    
    map.insert("alice", 25);
    map.insert("bob", 30);
    map.insert("charlie", 35);
    
    int value;
    EXPECT_TRUE(map.get("alice", value));
    EXPECT_EQ(value, 25);
    
    EXPECT_TRUE(map.get("bob", value));
    EXPECT_EQ(value, 30);
    
    EXPECT_FALSE(map.get("david", value));
}

TEST(ConcurrentHashMapTest, Contains) {
    ConcurrentHashMap<std::string, int> map;
    
    map.insert("alice", 25);
    
    EXPECT_TRUE(map.contains("alice"));
    EXPECT_FALSE(map.contains("bob"));
}

TEST(ConcurrentHashMapTest, Remove) {
    ConcurrentHashMap<std::string, int> map;
    
    map.insert("alice", 25);
    map.insert("bob", 30);
    
    EXPECT_TRUE(map.remove("alice"));
    EXPECT_FALSE(map.contains("alice"));
    EXPECT_EQ(map.size(), 1);
    
    EXPECT_FALSE(map.remove("charlie"));
    EXPECT_EQ(map.size(), 1);
}

TEST(ConcurrentHashMapTest, Size) {
    ConcurrentHashMap<std::string, int> map;
    
    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.empty());
    
    map.insert("alice", 25);
    EXPECT_EQ(map.size(), 1);
    EXPECT_FALSE(map.empty());
    
    map.insert("bob", 30);
    map.insert("charlie", 35);
    EXPECT_EQ(map.size(), 3);
}

TEST(ConcurrentHashMapTest, Clear) {
    ConcurrentHashMap<std::string, int> map;
    
    map.insert("alice", 25);
    map.insert("bob", 30);
    
    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
    EXPECT_FALSE(map.contains("alice"));
}

TEST(ConcurrentHashMapTest, Keys) {
    ConcurrentHashMap<std::string, int> map;
    
    map.insert("alice", 25);
    map.insert("bob", 30);
    map.insert("charlie", 35);
    
    auto keys = map.keys();
    EXPECT_EQ(keys.size(), 3);
    
    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(keys[0], "alice");
    EXPECT_EQ(keys[1], "bob");
    EXPECT_EQ(keys[2], "charlie");
}

TEST(ConcurrentHashMapTest, ConcurrentInserts) {
    ConcurrentHashMap<int, int> map;
    const int num_threads = 4;
    const int items_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&map, t, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                int key = t * items_per_thread + i;
                map.insert(key, key * 10);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(map.size(), num_threads * items_per_thread);
    
    // Verify all elements
    for (int i = 0; i < num_threads * items_per_thread; ++i) {
        int value;
        EXPECT_TRUE(map.get(i, value));
        EXPECT_EQ(value, i * 10);
    }
}

TEST(ConcurrentHashMapTest, ConcurrentReadsWrites) {
    ConcurrentHashMap<int, int> map;
    
    // Pre-populate map
    for (int i = 0; i < 100; ++i) {
        map.insert(i, i);
    }
    
    std::vector<std::thread> threads;
    
    // Writer threads
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&map, t]() {
            for (int i = 0; i < 50; ++i) {
                map.insert(100 + t * 50 + i, i);
            }
        });
    }
    
    // Reader threads
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&map]() {
            for (int i = 0; i < 100; ++i) {
                int value;
                map.get(i, value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(map.size(), 200);
}

TEST(ConcurrentHashMapTest, ConcurrentRemoves) {
    ConcurrentHashMap<int, int> map;
    
    // Pre-populate map
    for (int i = 0; i < 100; ++i) {
        map.insert(i, i * 10);
    }
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&map, t]() {
            for (int i = t; i < 100; i += 4) {
                map.remove(i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(map.size(), 0);
}

TEST(ConcurrentHashMapTest, UpdateValue) {
    ConcurrentHashMap<std::string, int> map;
    
    map.insert("counter", 0);
    
    std::vector<std::thread> threads;
    const int num_threads = 10;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&map]() {
            for (int i = 0; i < 100; ++i) {
                int value;
                if (map.get("counter", value)) {
                    map.insert("counter", value + 1);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Note: This test just verifies thread safety, not atomicity of increment
    EXPECT_TRUE(map.contains("counter"));
    int final_value;
    map.get("counter", final_value);
    EXPECT_GT(final_value, 0);
}
