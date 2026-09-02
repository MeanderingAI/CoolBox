#include "tyst_framework.hpp"

#include "splay_tree.h"

#include <string>
#include <vector>

using namespace data_structures;

TEST(SplayTreeTest, InsertAndSearch) {
    SplayTree<int> tree;
    tree.insert(10);
    tree.insert(5);
    tree.insert(20);

    EXPECT_TRUE(tree.search(5));
    EXPECT_TRUE(tree.search(20));
    EXPECT_FALSE(tree.search(99));
}

TEST(SplayTreeTest, RemoveExistingValue) {
    SplayTree<int> tree;
    tree.insert(10);
    tree.insert(5);
    tree.insert(20);

    EXPECT_TRUE(tree.remove(10));
    EXPECT_FALSE(tree.search(10));
    EXPECT_EQ(tree.size(), 2u);
}

TEST(SplayTreeTest, ClearResetsTree) {
    SplayTree<int> tree;
    tree.insert(1);
    tree.insert(2);
    tree.clear();

    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0u);
}

TEST(SplayTreeTest, InorderTraversalRemainsSorted) {
    SplayTree<int> tree;
    tree.insert(3);
    tree.insert(1);
    tree.insert(4);
    tree.insert(2);

    std::vector<int> values;
    tree.inorder_traversal([&values](const int& value) { values.push_back(value); });

    ASSERT_EQ(values.size(), 4u);
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);
    EXPECT_EQ(values[3], 4);
}

TEST(SplayTreeTest, WorksWithStrings) {
    SplayTree<std::string> tree;
    tree.insert("beta");
    tree.insert("alpha");
    tree.insert("gamma");

    EXPECT_TRUE(tree.search("alpha"));
    EXPECT_TRUE(tree.remove("beta"));
}
