#include <iostream>
#include <iomanip>
#include "data_structures/binary_search_tree.h"
#include "data_structures/linked_list.h"
#include "data_structures/hash_map.h"

using namespace data_structures;

void demo_binary_search_tree() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Binary Search Tree Demo            ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    BinarySearchTree<int> bst;
    
    // Insert elements
    std::cout << "Inserting: 50, 30, 70, 20, 40, 60, 80\n";
    bst.insert(50);
    bst.insert(30);
    bst.insert(70);
    bst.insert(20);
    bst.insert(40);
    bst.insert(60);
    bst.insert(80);
    
    std::cout << "Tree size: " << bst.size() << "\n";
    std::cout << "Min value: " << bst.min() << "\n";
    std::cout << "Max value: " << bst.max() << "\n";
    
    // Traversals
    std::cout << "\nInorder traversal (sorted): ";
    bst.inorder_traversal([](const int& val) {
        std::cout << val << " ";
    });
    
    std::cout << "\nPreorder traversal: ";
    bst.preorder_traversal([](const int& val) {
        std::cout << val << " ";
    });
    
    std::cout << "\nPostorder traversal: ";
    bst.postorder_traversal([](const int& val) {
        std::cout << val << " ";
    });
    std::cout << "\n";
    
    // Search operations
    std::cout << "\nSearching for 40: " << (bst.search(40) ? "Found" : "Not found") << "\n";
    std::cout << "Searching for 100: " << (bst.search(100) ? "Found" : "Not found") << "\n";
    
    // Remove operations
    std::cout << "\nRemoving 30...\n";
    bst.remove(30);
    std::cout << "Tree size after removal: " << bst.size() << "\n";
    std::cout << "Inorder traversal: ";
    bst.inorder_traversal([](const int& val) {
        std::cout << val << " ";
    });
    std::cout << "\n";
}

void demo_linked_list() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Linked List Demo                    ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    LinkedList<std::string> list;
    
    // Push operations
    std::cout << "Adding elements: Alice, Bob, Charlie\n";
    list.push_back("Alice");
    list.push_back("Bob");
    list.push_back("Charlie");
    
    std::cout << "List size: " << list.size() << "\n";
    std::cout << "Front: " << list.front() << "\n";
    std::cout << "Back: " << list.back() << "\n";
    
    std::cout << "\nList contents: ";
    list.for_each([](const std::string& val) {
        std::cout << val << " -> ";
    });
    std::cout << "null\n";
    
    // Insert at position
    std::cout << "\nInserting 'David' at position 1...\n";
    list.insert_at(1, "David");
    
    std::cout << "List contents: ";
    list.for_each([](const std::string& val) {
        std::cout << val << " -> ";
    });
    std::cout << "null\n";
    
    // Reverse
    std::cout << "\nReversing list...\n";
    list.reverse();
    std::cout << "Reversed list: ";
    list.for_each([](const std::string& val) {
        std::cout << val << " -> ";
    });
    std::cout << "null\n";
    
    // Pop operations
    std::cout << "\nPopping front element...\n";
    list.pop_front();
    std::cout << "List size: " << list.size() << "\n";
    std::cout << "New front: " << list.front() << "\n";
}

void demo_doubly_linked_list() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Doubly Linked List Demo             ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    DoublyLinkedList<int> dlist;
    
    std::cout << "Building list: ";
    for (int i = 1; i <= 5; ++i) {
        dlist.push_back(i * 10);
        std::cout << (i * 10) << " ";
    }
    std::cout << "\n";
    
    std::cout << "Size: " << dlist.size() << "\n";
    std::cout << "Front: " << dlist.front() << "\n";
    std::cout << "Back: " << dlist.back() << "\n";
    
    std::cout << "\nAdding 5 to front and 60 to back...\n";
    dlist.push_front(5);
    dlist.push_back(60);
    
    std::cout << "New front: " << dlist.front() << "\n";
    std::cout << "New back: " << dlist.back() << "\n";
    std::cout << "Size: " << dlist.size() << "\n";
}

void demo_hash_map() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HashMap Demo                        ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    HashMap<std::string, int> ages;
    
    // Insert key-value pairs
    std::cout << "Adding people and ages:\n";
    ages.insert("Alice", 25);
    ages.insert("Bob", 30);
    ages.insert("Charlie", 35);
    ages.insert("Diana", 28);
    
    std::cout << "HashMap size: " << ages.size() << "\n";
    
    // Access values
    std::cout << "\nLooking up ages:\n";
    int age;
    if (ages.get("Alice", age)) {
        std::cout << "Alice's age: " << age << "\n";
    }
    if (ages.get("Bob", age)) {
        std::cout << "Bob's age: " << age << "\n";
    }
    
    // Using operator[]
    std::cout << "Charlie's age (using []): " << ages["Charlie"] << "\n";
    
    // Check existence
    std::cout << "\nChecking existence:\n";
    std::cout << "Contains 'Diana': " << (ages.contains("Diana") ? "Yes" : "No") << "\n";
    std::cout << "Contains 'Eve': " << (ages.contains("Eve") ? "Yes" : "No") << "\n";
    
    // List all keys
    std::cout << "\nAll people in the map:\n";
    auto keys = ages.keys();
    for (const auto& key : keys) {
        std::cout << "  " << key << ": " << ages[key] << " years old\n";
    }
    
    // Remove
    std::cout << "\nRemoving Bob...\n";
    ages.remove("Bob");
    std::cout << "HashMap size after removal: " << ages.size() << "\n";
    
    // Update value
    std::cout << "\nUpdating Alice's age to 26...\n";
    ages["Alice"] = 26;
    std::cout << "Alice's new age: " << ages["Alice"] << "\n";
}

void demo_performance() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Performance Comparison              ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    const int N = 10000;
    
    // BST performance
    auto start = std::chrono::steady_clock::now();
    BinarySearchTree<int> bst;
    for (int i = 0; i < N; ++i) {
        bst.insert(rand() % (N * 10));
    }
    auto end = std::chrono::steady_clock::now();
    auto bst_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // HashMap performance
    start = std::chrono::steady_clock::now();
    HashMap<int, int> map;
    for (int i = 0; i < N; ++i) {
        map.insert(i, i * 2);
    }
    end = std::chrono::steady_clock::now();
    auto map_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // LinkedList performance
    start = std::chrono::steady_clock::now();
    LinkedList<int> list;
    for (int i = 0; i < N; ++i) {
        list.push_back(i);
    }
    end = std::chrono::steady_clock::now();
    auto list_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Inserting " << N << " elements:\n\n";
    std::cout << "  BST:        " << std::setw(8) << bst_time.count() << " μs\n";
    std::cout << "  HashMap:    " << std::setw(8) << map_time.count() << " μs\n";
    std::cout << "  LinkedList: " << std::setw(8) << list_time.count() << " μs\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║       Data Structures Library Demo                ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    demo_binary_search_tree();
    demo_linked_list();
    demo_doubly_linked_list();
    demo_hash_map();
    demo_performance();
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Demo Complete!                      ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    return 0;
}
