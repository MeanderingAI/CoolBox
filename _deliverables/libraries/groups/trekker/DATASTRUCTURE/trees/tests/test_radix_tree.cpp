#include <tyst_framework.hpp>
#include "radix_tree.h"

using namespace tyst::framework;

TEST(RadixTreeTest, BasicInsertFind) {
    radix_tree<int> tree;
    tree.insert("one", 100);
    tree.insert("two", 200);
    EXPECT_TRUE(tree.find("one").has_value());
    EXPECT_TRUE(tree.find("two").has_value());
    EXPECT_FALSE(tree.find("three").has_value());
    EXPECT_EQ(tree.find("one").value(), 100);
    EXPECT_EQ(tree.find("two").value(), 200);
}

TEST(RadixTreeTest, Remove) {
    radix_tree<int> tree;
    tree.insert("one", 100);
    tree.insert("two", 200);
    EXPECT_TRUE(tree.remove("one"));
    EXPECT_FALSE(tree.find("one").has_value());
    EXPECT_TRUE(tree.find("two").has_value());
}

// No main() needed, tyst_framework provides it.
