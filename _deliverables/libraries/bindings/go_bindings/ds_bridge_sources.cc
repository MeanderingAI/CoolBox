#include "../../groups/trekker/DATASTRUCTURE/list/source/linked_list.cpp"
#include "../../groups/trekker/DATASTRUCTURE/list/source/concurrent_linked_list.cpp"
#include "../../groups/trekker/DATASTRUCTURE/maps/source/hash_map.cpp"
#include "../../groups/trekker/DATASTRUCTURE/maps/source/concurrent_hash_map.cpp"
#include "../../groups/trekker/DATASTRUCTURE/trees/source/binary_search_tree.cpp"
#include "../../groups/trekker/DATASTRUCTURE/trees/source/btree.cpp"
#include "../../groups/trekker/DATASTRUCTURE/trees/source/red_black_tree.cpp"
#include "../../groups/trekker/DATASTRUCTURE/trees/source/splay_tree.cpp"
#include "../../groups/trekker/DATASTRUCTURE/trees/source/heap.cpp"
#include "../../groups/trekker/DATASTRUCTURE/trees/source/log_structured_merge_tree.cpp"
#include "../../groups/trekker/DATASTRUCTURE/trees/source/van_emde_boas.cpp"

#include <cstdint>
#include <functional>

template class data_structures::DoublyLinkedList<std::int64_t>;
template class data_structures::LogStructuredMergeTree<std::int64_t, std::int64_t>;
template class data_structures::Heap<std::int64_t, std::greater<std::int64_t>>;

template class data_structures::LinkedList<std::int64_t>;
template class data_structures::ConcurrentLinkedList<std::int64_t>;
template class data_structures::HashMap<std::int64_t, std::int64_t>;
template class data_structures::ConcurrentHashMap<std::int64_t, std::int64_t>;
template class data_structures::BinarySearchTree<std::int64_t>;
template class data_structures::BPlusTree<std::int64_t, std::int64_t, 4>;
template class data_structures::RedBlackTree<std::int64_t>;
template class data_structures::SplayTree<std::int64_t>;
template class data_structures::Heap<std::int64_t>;
