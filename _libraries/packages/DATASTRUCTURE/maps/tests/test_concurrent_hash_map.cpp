#include "tyst_framework.hpp"
#include "concurrent_hash_map.h"
#include <thread>
#include <vector>

using namespace data_structures;

TEST(ConcurrentHashMapTest, InsertAndGet) {
    ConcurrentHashMap<int, int> map;
    map.insert(1, 100);
    map.insert(2, 200);
    int val = 0;
    EXPECT_TRUE(map.get(1, val));
    EXPECT_EQ(val, 100);
    EXPECT_TRUE(map.get(2, val));
    EXPECT_EQ(val, 200);
}

TEST(ConcurrentHashMapTest, Contains) {
    ConcurrentHashMap<int, int> map;
    map.insert(1, 100);
    EXPECT_TRUE(map.contains(1));
    EXPECT_FALSE(map.contains(2));
}

TEST(ConcurrentHashMapTest, Remove) {
    ConcurrentHashMap<int, int> map;
    map.insert(1, 100);
    EXPECT_TRUE(map.remove(1));
    EXPECT_FALSE(map.contains(1));
    EXPECT_EQ(map.size(), 0u);
}

TEST(ConcurrentHashMapTest, ConcurrentInserts) {
    ConcurrentHashMap<int, int> map;
    const int num_threads = 4;
    const int inserts_per_thread = 100;
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&map, t, inserts_per_thread]() {
            for (int i = 0; i < inserts_per_thread; ++i) {
                map.insert(t * inserts_per_thread + i, i);
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(map.size(), static_cast<size_t>(num_threads * inserts_per_thread));
}

TEST(ConcurrentHashMapTest, Clear) {
    ConcurrentHashMap<int, int> map;
    map.insert(1, 10);
    map.insert(2, 20);
    map.clear();
    EXPECT_TRUE(map.empty());
}
