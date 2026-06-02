#include <tyst_framework.hpp>
#include "maple_tree.h"

using namespace tyst::framework;

TEST(MapleTreeTest, BasicInsertFind) {
    MapleTree<int, int> tree;
    tree.insert(1, 100);
    tree.insert(2, 200);
    EXPECT_TRUE(tree.find(1).has_value());
    EXPECT_TRUE(tree.find(2).has_value());
    EXPECT_FALSE(tree.find(3).has_value());
    EXPECT_EQ(tree[1], 100);
    EXPECT_EQ(tree[2], 200);
}

TEST(MapleTreeTest, Remove) {
    MapleTree<int, int> tree;
    tree.insert(1, 100);
    tree.insert(2, 200);
    EXPECT_TRUE(tree.remove(1));
    EXPECT_FALSE(tree.find(1).has_value());
    EXPECT_TRUE(tree.find(2).has_value());
}

// No main() needed, tyst_framework provides it.
