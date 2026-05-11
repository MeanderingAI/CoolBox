// test_radix_tree.cpp
// Basic tests for radix_tree
#include "radix_tree.h"
#include <cassert>
#include <iostream>

int main() {
    radix_tree<int> tree;
    tree.insert("apple", 1);
    tree.insert("app", 2);
    tree.insert("banana", 3);
    tree.insert("band", 4);
    tree.insert("bandana", 5);

    assert(tree.find("apple").value() == 1);
    assert(tree.find("app").value() == 2);
    assert(tree.find("banana").value() == 3);
    assert(tree.find("band").value() == 4);
    assert(tree.find("bandana").value() == 5);
    assert(!tree.find("ban").has_value());
    assert(!tree.find("apples").has_value());
    std::cout << "All radix_tree tests passed!\n";
    return 0;
}
