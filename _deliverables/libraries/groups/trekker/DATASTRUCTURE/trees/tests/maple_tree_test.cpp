// maple_tree_test.cpp
// Basic tests for MapleTree
#include "maple_tree.h"
#include <cassert>
#include <iostream>

int main() {
    MapleTree<int, std::string> tree;
    tree.insert(10, "ten");
    tree.insert(20, "twenty");
    tree.insert(5, "five");
    tree.insert(6, "six");
    tree.insert(12, "twelve");
    tree.insert(30, "thirty");
    tree.insert(7, "seven");
    tree.insert(17, "seventeen");

    assert(tree.find(10).value() == "ten");
    assert(tree.find(20).value() == "twenty");
    assert(tree.find(5).value() == "five");
    assert(tree.find(6).value() == "six");
    assert(tree.find(12).value() == "twelve");
    assert(tree.find(30).value() == "thirty");
    assert(tree.find(7).value() == "seven");
    assert(tree.find(17).value() == "seventeen");
    assert(!tree.find(100).has_value());
    std::cout << "All MapleTree tests passed!\n";
    return 0;
}
