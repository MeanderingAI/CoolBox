#include "tyst_framework.hpp"
#include "btree.h"
#include <string>
#include <vector>

using namespace data_structures;

// Basic insertion and search
TEST(BPlusTreeTest, InsertAndSearchSingleElement) {
	BPlusTree<std::string, std::string> tree;
	tree.insert("key1", "value1");
	auto result = tree.search("key1");
	ASSERT_TRUE(result.has_value());
	ASSERT_EQ(result.value(), "value1");
}

TEST(BPlusTreeTest, SearchNonexistentKey) {
	BPlusTree<std::string, std::string> tree;
	tree.insert("key1", "value1");
	auto result = tree.search("key2");
	ASSERT_FALSE(result.has_value());
}

TEST(BPlusTreeTest, InsertMultipleElements) {
	BPlusTree<std::string, std::string> tree;
	tree.insert("apple", "fruit");
	tree.insert("banana", "fruit");
	tree.insert("carrot", "vegetable");
	
	ASSERT_EQ(tree.size(), 3);
	ASSERT_EQ(tree.search("apple").value(), "fruit");
	ASSERT_EQ(tree.search("banana").value(), "fruit");
	ASSERT_EQ(tree.search("carrot").value(), "vegetable");
}

TEST(BPlusTreeTest, UpdateExistingKey) {
	BPlusTree<std::string, std::string> tree;
	tree.insert("key1", "value1");
	ASSERT_EQ(tree.size(), 1);
	
	tree.insert("key1", "updated_value");
	ASSERT_EQ(tree.size(), 1);
	ASSERT_EQ(tree.search("key1").value(), "updated_value");
}

// Range queries
TEST(BPlusTreeTest, RangeQueryBasic) {
	BPlusTree<int, std::string> tree;
	for (int i = 1; i <= 10; i++) {
		tree.insert(i, "value" + std::to_string(i));
	}
	
	auto range_results = tree.range_query(3, 7);
	ASSERT_EQ(range_results.size(), 5);
	ASSERT_EQ(range_results[0].first, 3);
	ASSERT_EQ(range_results[4].first, 7);
}

TEST(BPlusTreeTest, RangeQueryEmptyResult) {
	BPlusTree<int, std::string> tree;
	tree.insert(1, "a");
	tree.insert(10, "b");
	
	auto range_results = tree.range_query(2, 5);
	ASSERT_EQ(range_results.size(), 0);
}

TEST(BPlusTreeTest, RangeQueryEntireTree) {
	BPlusTree<int, std::string> tree;
	tree.insert(5, "e");
	tree.insert(2, "b");
	tree.insert(8, "h");
	tree.insert(1, "a");
	
	auto all = tree.range_query(1, 8);
	ASSERT_EQ(all.size(), 4);
}

// All entries
TEST(BPlusTreeTest, AllEntriesSorted) {
	BPlusTree<int, std::string> tree;
	tree.insert(5, "e");
	tree.insert(2, "b");
	tree.insert(8, "h");
	tree.insert(1, "a");
	tree.insert(3, "c");
	
	auto all = tree.all_entries();
	ASSERT_EQ(all.size(), 5);
	ASSERT_EQ(all[0].first, 1);
	ASSERT_EQ(all[1].first, 2);
	ASSERT_EQ(all[2].first, 3);
	ASSERT_EQ(all[3].first, 5);
	ASSERT_EQ(all[4].first, 8);
}

// Deletion
TEST(BPlusTreeTest, DeleteExistingKey) {
	BPlusTree<std::string, std::string> tree;
	tree.insert("key1", "value1");
	tree.insert("key2", "value2");
	tree.insert("key3", "value3");
	
	bool removed = tree.remove("key2");
	ASSERT_TRUE(removed);
	ASSERT_EQ(tree.size(), 2);
	ASSERT_FALSE(tree.search("key2").has_value());
}

TEST(BPlusTreeTest, DeleteNonexistentKey) {
	BPlusTree<std::string, std::string> tree;
	tree.insert("key1", "value1");
	
	bool removed = tree.remove("key2");
	ASSERT_FALSE(removed);
	ASSERT_EQ(tree.size(), 1);
}

TEST(BPlusTreeTest, DeleteAndSearchRemaining) {
	BPlusTree<int, std::string> tree;
	for (int i = 1; i <= 5; i++) {
		tree.insert(i, "v" + std::to_string(i));
	}
	
	tree.remove(3);
	ASSERT_EQ(tree.size(), 4);
	ASSERT_FALSE(tree.search(3).has_value());
	ASSERT_EQ(tree.search(2).value(), "v2");
	ASSERT_EQ(tree.search(4).value(), "v4");
}

// Stress tests
TEST(BPlusTreeTest, LargeInsertionAndSearch) {
	BPlusTree<int, int> tree;
	for (int i = 0; i < 100; i++) {
		tree.insert(i, i * 10);
	}
	
	ASSERT_EQ(tree.size(), 100);
	ASSERT_EQ(tree.search(50).value(), 500);
	ASSERT_EQ(tree.search(99).value(), 990);
}

TEST(BPlusTreeTest, LargeDeletion) {
	BPlusTree<int, int> tree;
	for (int i = 0; i < 50; i++) {
		tree.insert(i, i * 10);
	}
	
	for (int i = 0; i < 25; i++) {
		tree.remove(i);
	}
	
	ASSERT_EQ(tree.size(), 25);
	ASSERT_FALSE(tree.search(10).has_value());
	ASSERT_EQ(tree.search(30).value(), 300);
}

TEST(BPlusTreeTest, ClearTree) {
	BPlusTree<std::string, std::string> tree;
	tree.insert("a", "1");
	tree.insert("b", "2");
	tree.insert("c", "3");
	
	ASSERT_FALSE(tree.empty());
	tree.clear();
	ASSERT_TRUE(tree.empty());
	ASSERT_EQ(tree.size(), 0);
}

// Edge cases
TEST(BPlusTreeTest, OperationsOnEmptyTree) {
	BPlusTree<int, std::string> tree;
	
	ASSERT_TRUE(tree.empty());
	ASSERT_EQ(tree.size(), 0);
	ASSERT_FALSE(tree.search(1).has_value());
	ASSERT_FALSE(tree.remove(1));
}

TEST(BPlusTreeTest, DuplicateInsertions) {
	BPlusTree<std::string, std::string> tree;
	tree.insert("key", "value1");
	tree.insert("key", "value2");
	tree.insert("key", "value3");
	
	ASSERT_EQ(tree.size(), 1);
	ASSERT_EQ(tree.search("key").value(), "value3");
}

TEST(BPlusTreeTest, StringAsKeyAndValue) {
	BPlusTree<std::string, std::vector<std::string>> tree;
	std::vector<std::string> tags = {"tag1", "tag2", "tag3"};
	tree.insert("entry1", tags);
	
	auto result = tree.search("entry1");
	ASSERT_TRUE(result.has_value());
	ASSERT_EQ(result.value().size(), 3);
	ASSERT_EQ(result.value()[0], "tag1");
}

// Insertion order independence
TEST(BPlusTreeTest, InsertionOrderIndependenceAscending) {
	BPlusTree<int, int> tree;
	for (int i = 1; i <= 10; i++) {
		tree.insert(i, i);
	}
	
	auto all = tree.all_entries();
	ASSERT_EQ(all.size(), 10);
	for (int i = 0; i < 10; i++) {
		ASSERT_EQ(all[i].first, i + 1);
	}
}

TEST(BPlusTreeTest, InsertionOrderIndependenceDescending) {
	BPlusTree<int, int> tree;
	for (int i = 10; i >= 1; i--) {
		tree.insert(i, i);
	}
	
	auto all = tree.all_entries();
	ASSERT_EQ(all.size(), 10);
	for (int i = 0; i < 10; i++) {
		ASSERT_EQ(all[i].first, i + 1);
	}
}

TEST(BPlusTreeTest, InsertionOrderIndependenceRandom) {
	BPlusTree<int, int> tree;
	std::vector<int> keys = {5, 2, 8, 1, 9, 3, 7, 4, 6, 10};
	for (int key : keys) {
		tree.insert(key, key * 100);
	}
	
	auto all = tree.all_entries();
	ASSERT_EQ(all.size(), 10);
	for (int i = 0; i < 10; i++) {
		ASSERT_EQ(all[i].first, i + 1);
		ASSERT_EQ(all[i].second, (i + 1) * 100);
	}
}
