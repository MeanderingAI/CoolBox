#include <gtest/gtest.h>
#include "data_structures/linked_list.h"
#include <vector>

using namespace data_structures;

// LinkedList Tests
TEST(LinkedListTest, PushFront) {
    LinkedList<int> list;
    
    list.push_front(3);
    list.push_front(2);
    list.push_front(1);
    
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.front(), 1);
    EXPECT_EQ(list.back(), 3);
}

TEST(LinkedListTest, PushBack) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.front(), 1);
    EXPECT_EQ(list.back(), 3);
}

TEST(LinkedListTest, PopFront) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_TRUE(list.pop_front());
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.front(), 2);
    
    EXPECT_TRUE(list.pop_front());
    EXPECT_TRUE(list.pop_front());
    EXPECT_FALSE(list.pop_front());
    EXPECT_TRUE(list.empty());
}

TEST(LinkedListTest, PopBack) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_TRUE(list.pop_back());
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.back(), 2);
}

TEST(LinkedListTest, InsertAt) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(3);
    list.insert_at(1, 2);
    
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.at(0), 1);
    EXPECT_EQ(list.at(1), 2);
    EXPECT_EQ(list.at(2), 3);
}

TEST(LinkedListTest, RemoveAt) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_TRUE(list.remove_at(1));
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.at(0), 1);
    EXPECT_EQ(list.at(1), 3);
}

TEST(LinkedListTest, RemoveValue) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_TRUE(list.remove_value(2));
    EXPECT_EQ(list.size(), 2);
    EXPECT_FALSE(list.find(2));
    
    EXPECT_FALSE(list.remove_value(10));
}

TEST(LinkedListTest, Find) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_TRUE(list.find(2));
    EXPECT_FALSE(list.find(10));
}

TEST(LinkedListTest, ForEach) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    std::vector<int> result;
    list.for_each([&result](const int& val) {
        result.push_back(val);
    });
    
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

TEST(LinkedListTest, Reverse) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    list.reverse();
    
    EXPECT_EQ(list.front(), 3);
    EXPECT_EQ(list.back(), 1);
    EXPECT_EQ(list.at(0), 3);
    EXPECT_EQ(list.at(1), 2);
    EXPECT_EQ(list.at(2), 1);
}

TEST(LinkedListTest, Clear) {
    LinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    list.clear();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);
}

// DoublyLinkedList Tests
TEST(DoublyLinkedListTest, PushFrontBack) {
    DoublyLinkedList<int> list;
    
    list.push_back(2);
    list.push_front(1);
    list.push_back(3);
    
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.front(), 1);
    EXPECT_EQ(list.back(), 3);
}

TEST(DoublyLinkedListTest, PopFrontBack) {
    DoublyLinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    EXPECT_TRUE(list.pop_back());
    EXPECT_EQ(list.back(), 2);
    EXPECT_EQ(list.size(), 2);
    
    EXPECT_TRUE(list.pop_front());
    EXPECT_EQ(list.front(), 2);
    EXPECT_EQ(list.size(), 1);
}

TEST(DoublyLinkedListTest, Clear) {
    DoublyLinkedList<int> list;
    
    list.push_back(1);
    list.push_back(2);
    
    list.clear();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);
}
