#include <tyst_framework.hpp>
#include "binary_search_tree.h"
#include <string>
#include <vector>

using namespace data_structures;

TEST(BinarySearchTreeTest, InsertAndSearch) {
    BinarySearchTree<int> bst;
    bst.insert(5);
    bst.insert(3);
    bst.insert(7);
    EXPECT_TRUE(bst.search(5));
    EXPECT_TRUE(bst.search(3));
    EXPECT_TRUE(bst.search(7));
    EXPECT_FALSE(bst.search(1));
}

TEST(BinarySearchTreeTest, Size) {
    BinarySearchTree<int> bst;
    EXPECT_EQ(bst.size(), 0u);
    EXPECT_TRUE(bst.empty());
    bst.insert(10);
    bst.insert(20);
    bst.insert(5);
    EXPECT_EQ(bst.size(), 3u);
    EXPECT_FALSE(bst.empty());
}

TEST(BinarySearchTreeTest, DuplicateInsert) {
    BinarySearchTree<int> bst;
    bst.insert(5);
    bst.insert(5);
    EXPECT_EQ(bst.size(), 1u);
}

TEST(BinarySearchTreeTest, Remove) {
    BinarySearchTree<int> bst;
    bst.insert(5);
    bst.insert(3);
    bst.insert(7);
    EXPECT_TRUE(bst.remove(3));
    EXPECT_FALSE(bst.search(3));
    EXPECT_EQ(bst.size(), 2u);
    EXPECT_FALSE(bst.remove(100));
}

TEST(BinarySearchTreeTest, MinMax) {
    BinarySearchTree<int> bst;
    bst.insert(5);
    bst.insert(3);
    bst.insert(7);
    bst.insert(1);
    bst.insert(9);
    EXPECT_EQ(bst.min(), 1);
    EXPECT_EQ(bst.max(), 9);
}

TEST(BinarySearchTreeTest, InorderTraversal) {
    BinarySearchTree<int> bst;
    bst.insert(5);
    bst.insert(3);
    bst.insert(7);
    bst.insert(1);
    bst.insert(4);
    std::vector<int> result;
    bst.inorder_traversal([&result](const int& v) { result.push_back(v); });
    ASSERT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[1], 3);
    EXPECT_EQ(result[2], 4);
    EXPECT_EQ(result[3], 5);
    EXPECT_EQ(result[4], 7);
}

TEST(BinarySearchTreeTest, Clear) {
    BinarySearchTree<int> bst;
    bst.insert(1);
    bst.insert(2);
    bst.clear();
    EXPECT_TRUE(bst.empty());
    EXPECT_EQ(bst.size(), 0u);
}

TEST(BinarySearchTreeTest, StringType) {
    BinarySearchTree<std::string> bst;
    bst.insert("banana");
    bst.insert("apple");
    bst.insert("cherry");
    EXPECT_TRUE(bst.search("apple"));
    EXPECT_EQ(bst.min(), "apple");
    EXPECT_EQ(bst.max(), "cherry");
}
