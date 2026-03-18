#include <gtest/gtest.h>

#include "heap.h"

#include <string>

using namespace data_structures;

TEST(HeapTest, MaintainsMaxHeapOrdering) {
    Heap<int> heap;
    heap.push(4);
    heap.push(10);
    heap.push(7);
    heap.push(1);

    EXPECT_EQ(heap.top(), 10);
    EXPECT_TRUE(heap.pop());
    EXPECT_EQ(heap.top(), 7);
    EXPECT_TRUE(heap.pop());
    EXPECT_EQ(heap.top(), 4);
}

TEST(HeapTest, WorksWithStrings) {
    Heap<std::string> heap;
    heap.push("apple");
    heap.push("pear");
    heap.push("banana");

    EXPECT_EQ(heap.top(), "pear");
}

TEST(HeapTest, PopOnEmptyHeapFailsGracefully) {
    Heap<int> heap;
    EXPECT_FALSE(heap.pop());
}

TEST(HeapTest, ClearRemovesAllEntries) {
    Heap<int> heap;
    heap.push(1);
    heap.push(2);
    heap.clear();

    EXPECT_TRUE(heap.empty());
    EXPECT_EQ(heap.size(), 0u);
}
