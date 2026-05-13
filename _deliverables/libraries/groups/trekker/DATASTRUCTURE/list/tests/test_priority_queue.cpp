#include "tyst_framework.hpp"
#include "priority_queue.h"

#include <string>

using namespace data_structures;

// ═════════════════════════════════════════════════════════════════════════
// BinaryHeap — max-heap (default)
// ═════════════════════════════════════════════════════════════════════════

TEST(BinaryHeapTest, MaxHeapOrdering) {
    BinaryHeap<int> heap;
    heap.push(3);
    heap.push(1);
    heap.push(7);
    heap.push(4);
    heap.push(9);
    heap.push(2);

    std::vector<int> order;
    while (!heap.empty()) {
        order.push_back(heap.top());
        heap.pop();
    }
    // Should come out in descending order
    EXPECT_EQ(order, (std::vector<int>{9, 7, 4, 3, 2, 1}));
}

TEST(BinaryHeapTest, MinHeapOrdering) {
    BinaryHeap<int, std::greater<int>> heap;
    for (int v : {5, 2, 8, 1, 4}) heap.push(v);

    std::vector<int> order;
    while (!heap.empty()) {
        order.push_back(heap.top());
        heap.pop();
    }
    EXPECT_EQ(order, (std::vector<int>{1, 2, 4, 5, 8}));
}

TEST(BinaryHeapTest, SingleElement) {
    BinaryHeap<int> heap;
    heap.push(42);
    EXPECT_EQ(heap.top(), 42);
    EXPECT_EQ(heap.size(), 1u);
    heap.pop();
    EXPECT_TRUE(heap.empty());
}

TEST(BinaryHeapTest, PopOnEmptyThrows) {
    BinaryHeap<int> heap;
    EXPECT_THROW(heap.pop(), std::underflow_error);
}

TEST(BinaryHeapTest, TopOnEmptyThrows) {
    BinaryHeap<int> heap;
    EXPECT_THROW(heap.top(), std::underflow_error);
}

TEST(BinaryHeapTest, Clear) {
    BinaryHeap<int> heap;
    heap.push(1);
    heap.push(2);
    heap.clear();
    EXPECT_TRUE(heap.empty());
    EXPECT_EQ(heap.size(), 0u);
}

TEST(BinaryHeapTest, MoveInsert) {
    BinaryHeap<std::string> heap;
    std::string s = "hello";
    heap.push(std::move(s));
    EXPECT_EQ(heap.top(), "hello");
}

TEST(BinaryHeapTest, DuplicateValues) {
    BinaryHeap<int> heap;
    heap.push(5);
    heap.push(5);
    heap.push(5);
    EXPECT_EQ(heap.size(), 3u);
    heap.pop();
    EXPECT_EQ(heap.top(), 5);
}

// ═════════════════════════════════════════════════════════════════════════
// IndexedPriorityQueue — min-heap by priority (default)
// ═════════════════════════════════════════════════════════════════════════

TEST(IndexedPQTest, BasicMinHeap) {
    IndexedPriorityQueue<std::string, int> ipq;
    ipq.push("a", 5);
    ipq.push("b", 2);
    ipq.push("c", 8);
    ipq.push("d", 1);

    auto [k, p] = ipq.top();
    EXPECT_EQ(k, "d");
    EXPECT_EQ(p, 1);
}

TEST(IndexedPQTest, PopOrder) {
    IndexedPriorityQueue<int, int> ipq;
    ipq.push(10, 30);
    ipq.push(20, 10);
    ipq.push(30, 20);

    std::vector<int> keys;
    while (!ipq.empty()) {
        keys.push_back(ipq.top().first);
        ipq.pop();
    }
    EXPECT_EQ(keys, (std::vector<int>{20, 30, 10}));
}

TEST(IndexedPQTest, Contains) {
    IndexedPriorityQueue<std::string, int> ipq;
    ipq.push("x", 4);
    EXPECT_TRUE(ipq.contains("x"));
    EXPECT_FALSE(ipq.contains("y"));
}

TEST(IndexedPQTest, UpdateDecreasePriority) {
    IndexedPriorityQueue<std::string, int> ipq;
    ipq.push("a", 10);
    ipq.push("b", 5);
    ipq.push("c", 8);

    // Decrease "a"'s priority so it becomes the minimum
    ipq.update("a", 1);
    auto [k, p] = ipq.top();
    EXPECT_EQ(k, "a");
    EXPECT_EQ(p, 1);
}

TEST(IndexedPQTest, UpdateIncreasePriority) {
    IndexedPriorityQueue<std::string, int> ipq;
    ipq.push("a", 1);
    ipq.push("b", 3);
    ipq.push("c", 2);

    // Increase "a"'s priority so it is no longer the minimum
    ipq.update("a", 10);
    auto [k, p] = ipq.top();
    EXPECT_EQ(k, "c");
    EXPECT_EQ(p, 2);
}

TEST(IndexedPQTest, PushDuplicateKeyUpdates) {
    IndexedPriorityQueue<int, int> ipq;
    ipq.push(1, 100);
    ipq.push(1, 50);  // same key — should update, not duplicate
    EXPECT_EQ(ipq.size(), 1u);
    EXPECT_EQ(ipq.top().second, 50);
}

TEST(IndexedPQTest, PopOnEmptyThrows) {
    IndexedPriorityQueue<int, int> ipq;
    EXPECT_THROW(ipq.pop(), std::underflow_error);
}

TEST(IndexedPQTest, TopOnEmptyThrows) {
    IndexedPriorityQueue<int, int> ipq;
    EXPECT_THROW(ipq.top(), std::underflow_error);
}

TEST(IndexedPQTest, Clear) {
    IndexedPriorityQueue<std::string, int> ipq;
    ipq.push("a", 1);
    ipq.push("b", 2);
    ipq.clear();
    EXPECT_TRUE(ipq.empty());
    EXPECT_FALSE(ipq.contains("a"));
}

TEST(IndexedPQTest, MaxHeapVariant) {
    IndexedPriorityQueue<int, int, std::less<int>> ipq;
    ipq.push(1, 5);
    ipq.push(2, 9);
    ipq.push(3, 3);
    EXPECT_EQ(ipq.top().first, 2);  // highest priority first
}

TEST(IndexedPQTest, FullPopSequence) {
    IndexedPriorityQueue<std::string, int> ipq;
    ipq.push("node_a", 4);
    ipq.push("node_b", 1);
    ipq.push("node_c", 3);
    ipq.push("node_d", 2);

    std::vector<std::string> order;
    while (!ipq.empty()) {
        order.push_back(ipq.top().first);
        ipq.pop();
    }
    EXPECT_EQ(order, (std::vector<std::string>{"node_b", "node_d", "node_c", "node_a"}));
}
