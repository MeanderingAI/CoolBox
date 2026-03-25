#include <gtest/gtest.h>
#include "linked_list.h"

using namespace data_structures;

TEST(LinkedListTest, PushFrontAndBack) {
    LinkedList<int> list;
    list.push_front(1);
    list.push_back(2);
    list.push_front(0);
    EXPECT_EQ(list.size(), 3u);
    EXPECT_EQ(list.front(), 0);
    EXPECT_EQ(list.back(), 2);
}

TEST(LinkedListTest, PopFrontAndBack) {
    LinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    EXPECT_TRUE(list.pop_front());
    EXPECT_EQ(list.front(), 2);
    EXPECT_TRUE(list.pop_back());
    EXPECT_EQ(list.back(), 2);
    EXPECT_EQ(list.size(), 1u);
}

TEST(LinkedListTest, Find) {
    LinkedList<int> list;
    list.push_back(10);
    list.push_back(20);
    list.push_back(30);
    EXPECT_TRUE(list.find(20));
    EXPECT_FALSE(list.find(99));
}

TEST(LinkedListTest, RemoveValue) {
    LinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    EXPECT_TRUE(list.remove_value(2));
    EXPECT_FALSE(list.find(2));
    EXPECT_EQ(list.size(), 2u);
}

TEST(LinkedListTest, Clear) {
    LinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.clear();
    EXPECT_TRUE(list.empty());
}

TEST(LinkedListTest, Reverse) {
    LinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    list.reverse();
    EXPECT_EQ(list.front(), 3);
    EXPECT_EQ(list.back(), 1);
}

TEST(DoublyLinkedListTest, PushFrontAndBack) {
    DoublyLinkedList<int> list;
    list.push_front(1);
    list.push_back(2);
    list.push_front(0);
    EXPECT_EQ(list.size(), 3u);
    EXPECT_EQ(list.front(), 0);
    EXPECT_EQ(list.back(), 2);
}

TEST(DoublyLinkedListTest, PopFrontAndBack) {
    DoublyLinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    EXPECT_TRUE(list.pop_front());
    EXPECT_EQ(list.front(), 2);
    EXPECT_TRUE(list.pop_back());
    EXPECT_EQ(list.back(), 2);
    EXPECT_EQ(list.size(), 1u);
}

TEST(DoublyLinkedListTest, Clear) {
    DoublyLinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.clear();
    EXPECT_TRUE(list.empty());
}
