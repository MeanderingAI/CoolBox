#include "tyst_framework.hpp"
#include "van_emde_boas.h"

using namespace data_structures;

// ── Basic insert / contains ───────────────────────────────────────────────

TEST(VanEmdeBoasTest, EmptyTree) {
    VanEmdeBoasTree veb(16);
    EXPECT_TRUE(veb.empty());
    EXPECT_EQ(veb.minimum(), VanEmdeBoasTree::NONE);
    EXPECT_EQ(veb.maximum(), VanEmdeBoasTree::NONE);
}

TEST(VanEmdeBoasTest, InsertAndContains) {
    VanEmdeBoasTree veb(16);
    veb.insert(3);
    veb.insert(7);
    veb.insert(12);
    EXPECT_TRUE(veb.contains(3));
    EXPECT_TRUE(veb.contains(7));
    EXPECT_TRUE(veb.contains(12));
    EXPECT_FALSE(veb.contains(0));
    EXPECT_FALSE(veb.contains(15));
}

TEST(VanEmdeBoasTest, MinMax) {
    VanEmdeBoasTree veb(32);
    veb.insert(10);
    veb.insert(2);
    veb.insert(25);
    veb.insert(5);
    EXPECT_EQ(veb.minimum(), 2u);
    EXPECT_EQ(veb.maximum(), 25u);
}

TEST(VanEmdeBoasTest, InsertDuplicateNoEffect) {
    VanEmdeBoasTree veb(16);
    veb.insert(5);
    veb.insert(5);
    EXPECT_EQ(veb.minimum(), 5u);
    EXPECT_EQ(veb.maximum(), 5u);
}

// ── Remove ───────────────────────────────────────────────────────────────

TEST(VanEmdeBoasTest, RemoveSingleElement) {
    VanEmdeBoasTree veb(8);
    veb.insert(4);
    veb.remove(4);
    EXPECT_TRUE(veb.empty());
    EXPECT_EQ(veb.minimum(), VanEmdeBoasTree::NONE);
}

TEST(VanEmdeBoasTest, RemoveMin) {
    VanEmdeBoasTree veb(16);
    veb.insert(1);
    veb.insert(5);
    veb.insert(9);
    veb.remove(1);
    EXPECT_FALSE(veb.contains(1));
    EXPECT_EQ(veb.minimum(), 5u);
}

TEST(VanEmdeBoasTest, RemoveMax) {
    VanEmdeBoasTree veb(16);
    veb.insert(1);
    veb.insert(5);
    veb.insert(9);
    veb.remove(9);
    EXPECT_FALSE(veb.contains(9));
    EXPECT_EQ(veb.maximum(), 5u);
}

TEST(VanEmdeBoasTest, RemoveMiddle) {
    VanEmdeBoasTree veb(16);
    veb.insert(2);
    veb.insert(6);
    veb.insert(10);
    veb.remove(6);
    EXPECT_FALSE(veb.contains(6));
    EXPECT_TRUE(veb.contains(2));
    EXPECT_TRUE(veb.contains(10));
}

TEST(VanEmdeBoasTest, RemoveNonExistentNoOp) {
    VanEmdeBoasTree veb(16);
    veb.insert(3);
    veb.remove(7); // not present
    EXPECT_TRUE(veb.contains(3));
    EXPECT_EQ(veb.minimum(), 3u);
}

// ── Successor ────────────────────────────────────────────────────────────

TEST(VanEmdeBoasTest, Successor) {
    VanEmdeBoasTree veb(32);
    for (uint64_t v : {1u, 5u, 9u, 14u, 20u}) veb.insert(v);

    EXPECT_EQ(veb.successor(0),  1u);
    EXPECT_EQ(veb.successor(1),  5u);
    EXPECT_EQ(veb.successor(5),  9u);
    EXPECT_EQ(veb.successor(9),  14u);
    EXPECT_EQ(veb.successor(14), 20u);
    EXPECT_EQ(veb.successor(20), VanEmdeBoasTree::NONE);
}

TEST(VanEmdeBoasTest, SuccessorBelowMin) {
    VanEmdeBoasTree veb(16);
    veb.insert(8);
    EXPECT_EQ(veb.successor(0), 8u);
}

// ── Predecessor ──────────────────────────────────────────────────────────

TEST(VanEmdeBoasTest, Predecessor) {
    VanEmdeBoasTree veb(32);
    for (uint64_t v : {2u, 6u, 11u, 17u, 22u}) veb.insert(v);

    EXPECT_EQ(veb.predecessor(30), 22u);
    EXPECT_EQ(veb.predecessor(22), 17u);
    EXPECT_EQ(veb.predecessor(17), 11u);
    EXPECT_EQ(veb.predecessor(11), 6u);
    EXPECT_EQ(veb.predecessor(6),  2u);
    EXPECT_EQ(veb.predecessor(2),  VanEmdeBoasTree::NONE);
}

// ── Larger universe ───────────────────────────────────────────────────────

TEST(VanEmdeBoasTest, LargeUniverse) {
    VanEmdeBoasTree veb(1024);
    veb.insert(0);
    veb.insert(511);
    veb.insert(1023);
    EXPECT_EQ(veb.minimum(), 0u);
    EXPECT_EQ(veb.maximum(), 1023u);
    EXPECT_EQ(veb.successor(0),    511u);
    EXPECT_EQ(veb.successor(511),  1023u);
    EXPECT_EQ(veb.predecessor(1023), 511u);
    veb.remove(511);
    EXPECT_EQ(veb.successor(0), 1023u);
}

// ── Out-of-range insert silently ignored ──────────────────────────────────

TEST(VanEmdeBoasTest, OutOfRangeInsertIgnored) {
    VanEmdeBoasTree veb(8);
    veb.insert(8);  // universe is [0,8)
    EXPECT_TRUE(veb.empty());
}
