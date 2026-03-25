#include <gtest/gtest.h>

#include "log_structured_merge_tree.h"

#include <string>

using namespace data_structures;

TEST(LogStructuredMergeTreeTest, PutAndGetValues) {
    LogStructuredMergeTree<std::string, int> tree(4);
    tree.put("alpha", 1);
    tree.put("beta", 2);

    int value = 0;
    EXPECT_TRUE(tree.get("alpha", value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(tree.get("beta", value));
    EXPECT_EQ(value, 2);
}

TEST(LogStructuredMergeTreeTest, FlushCreatesSSTable) {
    LogStructuredMergeTree<int, int> tree(2);
    tree.put(1, 10);
    tree.put(2, 20);

    EXPECT_EQ(tree.memtable_size(), 0u);
    EXPECT_EQ(tree.sstable_count(), 1u);
    EXPECT_TRUE(tree.contains(1));
    EXPECT_TRUE(tree.contains(2));
}

TEST(LogStructuredMergeTreeTest, RemoveMarksDeletion) {
    LogStructuredMergeTree<std::string, std::string> tree(8);
    tree.put("key", "value");

    EXPECT_TRUE(tree.remove("key"));
    EXPECT_FALSE(tree.contains("key"));
    EXPECT_EQ(tree.size(), 0u);
}

TEST(LogStructuredMergeTreeTest, CompactMergesSSTables) {
    LogStructuredMergeTree<int, int> tree(2);
    tree.put(1, 10);
    tree.put(2, 20);
    tree.put(1, 15);
    tree.put(3, 30);

    EXPECT_GE(tree.sstable_count(), 2u);
    tree.compact();
    EXPECT_EQ(tree.sstable_count(), 1u);

    int value = 0;
    EXPECT_TRUE(tree.get(1, value));
    EXPECT_EQ(value, 15);
}

TEST(LogStructuredMergeTreeTest, ClearResetsState) {
    LogStructuredMergeTree<int, int> tree(2);
    tree.put(1, 10);
    tree.put(2, 20);
    tree.clear();

    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.memtable_size(), 0u);
    EXPECT_EQ(tree.sstable_count(), 0u);
}
