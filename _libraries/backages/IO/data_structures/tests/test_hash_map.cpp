#include <gtest/gtest.h>
#include "hash_map.h"
#include <string>

using namespace data_structures;

TEST(HashMapTest, InsertAndGet) {
    HashMap<std::string, int> map;
    map.insert("one", 1);
    map.insert("two", 2);
    int val = 0;
    EXPECT_TRUE(map.get("one", val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(map.get("two", val));
    EXPECT_EQ(val, 2);
}

TEST(HashMapTest, Contains) {
    HashMap<int, int> map;
    map.insert(1, 100);
    EXPECT_TRUE(map.contains(1));
    EXPECT_FALSE(map.contains(2));
}

TEST(HashMapTest, Remove) {
    HashMap<int, int> map;
    map.insert(1, 100);
    map.insert(2, 200);
    EXPECT_TRUE(map.remove(1));
    EXPECT_FALSE(map.contains(1));
    EXPECT_EQ(map.size(), 1u);
    EXPECT_FALSE(map.remove(99));
}

TEST(HashMapTest, OverwriteValue) {
    HashMap<std::string, std::string> map;
    map.insert("key", "old");
    map.insert("key", "new");
    std::string val;
    EXPECT_TRUE(map.get("key", val));
    EXPECT_EQ(val, "new");
}

TEST(HashMapTest, SubscriptOperator) {
    HashMap<int, int> map;
    map.insert(1, 10);
    EXPECT_EQ(map[1], 10);
}

TEST(HashMapTest, KeysAndValues) {
    HashMap<int, int> map;
    map.insert(1, 10);
    map.insert(2, 20);
    auto keys = map.keys();
    auto vals = map.values();
    EXPECT_EQ(keys.size(), 2u);
    EXPECT_EQ(vals.size(), 2u);
}

TEST(HashMapTest, Clear) {
    HashMap<int, int> map;
    map.insert(1, 10);
    map.insert(2, 20);
    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0u);
}
