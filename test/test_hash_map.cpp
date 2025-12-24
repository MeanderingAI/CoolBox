#include <gtest/gtest.h>
#include "data_structures/hash_map.h"
#include <algorithm>

using namespace data_structures;

TEST(HashMapTest, InsertAndGet) {
    HashMap<std::string, int> map;
    
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

TEST(HashMapTest, OperatorBracket) {
    HashMap<std::string, int> map;
    
    map["alice"] = 25;
    map["bob"] = 30;
    
    EXPECT_EQ(map["alice"], 25);
    EXPECT_EQ(map["bob"], 30);
    
    // Accessing non-existent key creates it with default value
    int value = map["charlie"];
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(map.contains("charlie"));
}

TEST(HashMapTest, Contains) {
    HashMap<std::string, int> map;
    
    map.insert("alice", 25);
    
    EXPECT_TRUE(map.contains("alice"));
    EXPECT_FALSE(map.contains("bob"));
}

TEST(HashMapTest, Remove) {
    HashMap<std::string, int> map;
    
    map.insert("alice", 25);
    map.insert("bob", 30);
    
    EXPECT_TRUE(map.remove("alice"));
    EXPECT_FALSE(map.contains("alice"));
    EXPECT_EQ(map.size(), 1);
    
    EXPECT_FALSE(map.remove("charlie"));
    EXPECT_EQ(map.size(), 1);
}

TEST(HashMapTest, Size) {
    HashMap<std::string, int> map;
    
    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.empty());
    
    map.insert("alice", 25);
    EXPECT_EQ(map.size(), 1);
    EXPECT_FALSE(map.empty());
    
    map.insert("bob", 30);
    map.insert("charlie", 35);
    EXPECT_EQ(map.size(), 3);
}

TEST(HashMapTest, Clear) {
    HashMap<std::string, int> map;
    
    map.insert("alice", 25);
    map.insert("bob", 30);
    
    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
    EXPECT_FALSE(map.contains("alice"));
}

TEST(HashMapTest, Keys) {
    HashMap<std::string, int> map;
    
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

TEST(HashMapTest, Values) {
    HashMap<std::string, int> map;
    
    map.insert("alice", 25);
    map.insert("bob", 30);
    map.insert("charlie", 35);
    
    auto values = map.values();
    EXPECT_EQ(values.size(), 3);
    
    std::sort(values.begin(), values.end());
    EXPECT_EQ(values[0], 25);
    EXPECT_EQ(values[1], 30);
    EXPECT_EQ(values[2], 35);
}

TEST(HashMapTest, UpdateValue) {
    HashMap<std::string, int> map;
    
    map.insert("alice", 25);
    EXPECT_EQ(map["alice"], 25);
    
    map.insert("alice", 26);
    EXPECT_EQ(map["alice"], 26);
    EXPECT_EQ(map.size(), 1);
}

TEST(HashMapTest, IntegerKeys) {
    HashMap<int, std::string> map;
    
    map.insert(1, "one");
    map.insert(2, "two");
    map.insert(3, "three");
    
    std::string value;
    EXPECT_TRUE(map.get(2, value));
    EXPECT_EQ(value, "two");
    
    EXPECT_TRUE(map.remove(2));
    EXPECT_FALSE(map.contains(2));
}

TEST(HashMapTest, Rehashing) {
    HashMap<int, int> map(4, 0.75f);  // Small initial capacity
    
    // Insert enough elements to trigger rehashing
    for (int i = 0; i < 20; ++i) {
        map.insert(i, i * 10);
    }
    
    EXPECT_EQ(map.size(), 20);
    
    // Verify all elements are still accessible
    for (int i = 0; i < 20; ++i) {
        int value;
        EXPECT_TRUE(map.get(i, value));
        EXPECT_EQ(value, i * 10);
    }
}
