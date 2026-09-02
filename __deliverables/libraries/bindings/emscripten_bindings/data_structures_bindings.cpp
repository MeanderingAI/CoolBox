#include <emscripten/bind.h>
#include "binary_search_tree.h"
#include "linked_list.h"
#include "hash_map.h"

using namespace emscripten;
using namespace data_structures;

EMSCRIPTEN_BINDINGS(data_structures_module) {
    // BinarySearchTree<int>
    class_<BinarySearchTree<int>>("BinarySearchTreeInt")
        .constructor<>()
        .function("insert", &BinarySearchTree<int>::insert)
        .function("remove", &BinarySearchTree<int>::remove)
        .function("search", &BinarySearchTree<int>::search)
        .function("size", &BinarySearchTree<int>::size)
        .function("empty", &BinarySearchTree<int>::empty)
        .function("clear", &BinarySearchTree<int>::clear)
        .function("min", &BinarySearchTree<int>::min)
        .function("max", &BinarySearchTree<int>::max)
    ;

    // BinarySearchTree<double>
    class_<BinarySearchTree<double>>("BinarySearchTreeDouble")
        .constructor<>()
        .function("insert", &BinarySearchTree<double>::insert)
        .function("remove", &BinarySearchTree<double>::remove)
        .function("search", &BinarySearchTree<double>::search)
        .function("size", &BinarySearchTree<double>::size)
        .function("empty", &BinarySearchTree<double>::empty)
        .function("clear", &BinarySearchTree<double>::clear)
        .function("min", &BinarySearchTree<double>::min)
        .function("max", &BinarySearchTree<double>::max)
    ;

    // LinkedList<int>
    class_<LinkedList<int>>("LinkedListInt")
        .constructor<>()
        .function("push_front", &LinkedList<int>::push_front)
        .function("push_back", &LinkedList<int>::push_back)
        .function("pop_front", &LinkedList<int>::pop_front)
        .function("pop_back", &LinkedList<int>::pop_back)
        .function("front", &LinkedList<int>::front)
        .function("back", &LinkedList<int>::back)
        .function("find", &LinkedList<int>::find)
        .function("size", &LinkedList<int>::size)
        .function("empty", &LinkedList<int>::empty)
        .function("clear", &LinkedList<int>::clear)
        .function("reverse", &LinkedList<int>::reverse)
    ;

    // HashMap<string, string>
    class_<HashMap<std::string, std::string>>("HashMapStringString")
        .constructor<>()
        .function("insert", &HashMap<std::string, std::string>::insert)
        .function("remove", &HashMap<std::string, std::string>::remove)
        .function("contains", &HashMap<std::string, std::string>::contains)
        .function("size", &HashMap<std::string, std::string>::size)
        .function("empty", &HashMap<std::string, std::string>::empty)
        .function("clear", &HashMap<std::string, std::string>::clear)
    ;

    // HashMap<string, int>
    class_<HashMap<std::string, int>>("HashMapStringInt")
        .constructor<>()
        .function("insert", &HashMap<std::string, int>::insert)
        .function("remove", &HashMap<std::string, int>::remove)
        .function("contains", &HashMap<std::string, int>::contains)
        .function("size", &HashMap<std::string, int>::size)
        .function("empty", &HashMap<std::string, int>::empty)
        .function("clear", &HashMap<std::string, int>::clear)
    ;
}
