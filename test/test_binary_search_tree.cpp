#include <gtest/gtest.h>
#include "data_structures/binary_search_tree.h"
#include <vector>

using namespace data_structures;

TEST(BinarySearchTreeTest, InsertAndSearch) {
    BinarySearchTree<int> bst;
    
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    bst.insert(20);
    bst.insert(40);
    bst.insert(60);
    bst.insert(80);
    
    EXPECT_TRUE(bst.search(50));
    EXPECT_TRUE(bst.search(30));
    EXPECT_TRUE(bst.search(70));
    EXPECT_TRUE(bst.search(20));
    EXPECT_FALSE(bst.search(100));
    EXPECT_FALSE(bst.search(25));
}

TEST(BinarySearchTreeTest, Size) {
    BinarySearchTree<int> bst;
    
    EXPECT_EQ(bst.size(), 0);
    EXPECT_TRUE(bst.empty());
    
    bst.insert(50);
    EXPECT_EQ(bst.size(), 1);
    EXPECT_FALSE(bst.empty());
    
    bst.insert(30);
    bst.insert(70);
    EXPECT_EQ(bst.size(), 3);
}

TEST(BinarySearchTreeTest, Remove) {
    BinarySearchTree<int> bst;
    
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    bst.insert(20);
    bst.insert(40);
    
    EXPECT_TRUE(bst.remove(30));
    EXPECT_FALSE(bst.search(30));
    EXPECT_EQ(bst.size(), 4);
    
    EXPECT_FALSE(bst.remove(100));
    EXPECT_EQ(bst.size(), 4);
}

TEST(BinarySearchTreeTest, MinMax) {
    BinarySearchTree<int> bst;
    
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    bst.insert(20);
    bst.insert(80);
    
    EXPECT_EQ(bst.min(), 20);
    EXPECT_EQ(bst.max(), 80);
}

TEST(BinarySearchTreeTest, InorderTraversal) {
    BinarySearchTree<int> bst;
    
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    bst.insert(20);
    bst.insert(40);
    bst.insert(60);
    bst.insert(80);
    
    std::vector<int> result;
    bst.inorder_traversal([&result](const int& val) {
        result.push_back(val);
    });
    
    std::vector<int> expected = {20, 30, 40, 50, 60, 70, 80};
    EXPECT_EQ(result, expected);
}

TEST(BinarySearchTreeTest, PreorderTraversal) {
    BinarySearchTree<int> bst;
    
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    
    std::vector<int> result;
    bst.preorder_traversal([&result](const int& val) {
        result.push_back(val);
    });
    
    std::vector<int> expected = {50, 30, 70};
    EXPECT_EQ(result, expected);
}

TEST(BinarySearchTreeTest, PostorderTraversal) {
    BinarySearchTree<int> bst;
    
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    
    std::vector<int> result;
    bst.postorder_traversal([&result](const int& val) {
        result.push_back(val);
    });
    
    std::vector<int> expected = {30, 70, 50};
    EXPECT_EQ(result, expected);
}

TEST(BinarySearchTreeTest, Clear) {
    BinarySearchTree<int> bst;
    
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    
    bst.clear();
    EXPECT_TRUE(bst.empty());
    EXPECT_EQ(bst.size(), 0);
    EXPECT_FALSE(bst.search(50));
}

TEST(BinarySearchTreeTest, StringType) {
    BinarySearchTree<std::string> bst;
    
    bst.insert("banana");
    bst.insert("apple");
    bst.insert("cherry");
    
    EXPECT_TRUE(bst.search("apple"));
    EXPECT_TRUE(bst.search("banana"));
    EXPECT_TRUE(bst.search("cherry"));
    EXPECT_FALSE(bst.search("date"));
    
    EXPECT_EQ(bst.min(), "apple");
    EXPECT_EQ(bst.max(), "cherry");
}

TEST(BinarySearchTreeTest, DuplicateInserts) {
    BinarySearchTree<int> bst;
    
    bst.insert(50);
    bst.insert(50);
    bst.insert(50);
    
    // Duplicates should not increase size
    EXPECT_EQ(bst.size(), 1);
}
