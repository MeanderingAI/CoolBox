# Data Structures Library

A comprehensive C++ library providing efficient implementations of fundamental data structures with both single-threaded and concurrent (thread-safe) versions.

## Features

### Tree Data Structures

#### Binary Search Tree (Unbalanced)
- **Header**: `data_structures/binary_search_tree.h`
- Basic unbalanced BST with O(log n) average case operations
- Operations: insert, remove, search, traversals (inorder, preorder, postorder)
- Find min/max elements
- Template-based for any comparable type

#### Red-Black Tree
- **Header**: `data_structures/red_black_tree.h`
- Self-balancing BST maintaining O(log n) worst-case operations
- Automatic rebalancing through rotations and color flipping
- Guaranteed balanced height
- Operations: insert, remove, search, traversal

#### Splay Tree
- **Header**: `data_structures/splay_tree.h`
- Self-adjusting BST that moves frequently accessed elements to root
- Excellent for workloads with locality of reference
- Amortized O(log n) operations
- No extra storage for balance information

### Linked Lists

#### Single-Threaded Linked List
- **Header**: `data_structures/linked_list.h`
- Singly linked list with tail pointer for O(1) append
- Operations: push_front, push_back, pop_front, pop_back, insert_at, remove_at
- Find, reverse, and iteration support
- Doubly linked list variant included

#### Concurrent Linked List
- **Header**: `data_structures/concurrent_linked_list.h`
- Thread-safe linked list with fine-grained locking
- Lock-based synchronization for safe concurrent access
- Operations: push_front, push_back, pop_front, remove_value, find
- Atomic size tracking

### Hash Maps

#### Single-Threaded Hash Map
- **Header**: `data_structures/hash_map.h`
- Hash table with separate chaining for collision resolution
- Automatic rehashing based on load factor
- Operations: insert, remove, get, contains
- Support for custom hash functions
- Key/value iteration

#### Concurrent Hash Map
- **Header**: `data_structures/concurrent_hash_map.h`
- Thread-safe hash map with bucket-level locking
- Shared/exclusive locks for read/write operations
- Fine-grained locking for better concurrency
- Operations: insert, remove, get, contains
- Thread-safe size tracking

## Usage Examples

### Binary Search Tree
```cpp
#include "data_structures/binary_search_tree.h"

data_structures::BinarySearchTree<int> bst;
bst.insert(50);
bst.insert(30);
bst.insert(70);

bool found = bst.search(30);  // true
int min = bst.min();           // 30

bst.inorder_traversal([](const int& val) {
    std::cout << val << " ";
});
```

### Red-Black Tree
```cpp
#include "data_structures/red_black_tree.h"

data_structures::RedBlackTree<std::string> rbt;
rbt.insert("apple");
rbt.insert("banana");
rbt.insert("cherry");

bool exists = rbt.search("banana");  // true
rbt.remove("apple");
```

### Linked List
```cpp
#include "data_structures/linked_list.h"

data_structures::LinkedList<int> list;
list.push_back(1);
list.push_back(2);
list.push_back(3);

list.reverse();
int front = list.front();  // 3

list.for_each([](const int& val) {
    std::cout << val << " ";
});
```

### Hash Map
```cpp
#include "data_structures/hash_map.h"

data_structures::HashMap<std::string, int> map;
map.insert("alice", 25);
map.insert("bob", 30);

int age;
if (map.get("alice", age)) {
    std::cout << "Alice is " << age << " years old\n";
}

map["charlie"] = 35;  // operator[] access
```

### Concurrent Hash Map (Thread-Safe)
```cpp
#include "data_structures/concurrent_hash_map.h"
#include <thread>

data_structures::ConcurrentHashMap<std::string, int> concurrent_map;

// Thread 1
std::thread t1([&]() {
    concurrent_map.insert("key1", 100);
});

// Thread 2
std::thread t2([&]() {
    int value;
    if (concurrent_map.get("key1", value)) {
        std::cout << "Found: " << value << "\n";
    }
});

t1.join();
t2.join();
```

### Concurrent Linked List (Thread-Safe)
```cpp
#include "data_structures/concurrent_linked_list.h"
#include <thread>

data_structures::ConcurrentLinkedList<int> list;

// Multiple threads can safely modify the list
std::thread producer([&]() {
    for (int i = 0; i < 100; ++i) {
        list.push_front(i);
    }
});

std::thread consumer([&]() {
    int value;
    while (list.pop_front(value)) {
        std::cout << value << "\n";
    }
});

producer.join();
consumer.join();
```

## Performance Characteristics

| Data Structure | Insert | Remove | Search | Space |
|---------------|---------|---------|---------|--------|
| BST (avg) | O(log n) | O(log n) | O(log n) | O(n) |
| BST (worst) | O(n) | O(n) | O(n) | O(n) |
| Red-Black Tree | O(log n) | O(log n) | O(log n) | O(n) |
| Splay Tree (amortized) | O(log n) | O(log n) | O(log n) | O(n) |
| Linked List | O(1) front, O(1) back | O(1) front, O(n) back | O(n) | O(n) |
| Hash Map (avg) | O(1) | O(1) | O(1) | O(n) |
| Hash Map (worst) | O(n) | O(n) | O(n) | O(n) |

## Thread Safety

- **Single-threaded versions**: No internal synchronization, not thread-safe
- **Concurrent versions**: Thread-safe with internal locking mechanisms
  - Hash Map: Bucket-level locking for fine-grained concurrency
  - Linked List: Node-level locking with atomic operations

## Building

The data structures library is automatically built as part of the main project:

```bash
make
```

The library will be created at: `build/src/data_structures/libdata_structures.dylib`

## Linking

To use the data structures library in your code:

```cmake
target_link_libraries(your_target PRIVATE data_structures)
```

## Template Instantiations

The library provides explicit instantiations for common types:
- `int`
- `double`
- `std::string`

For other types, include the header and the template will be instantiated automatically.

## Notes

- All data structures use smart pointers (`std::shared_ptr`, `std::unique_ptr`) for automatic memory management
- Concurrent data structures require linking with pthread (`-pthread`)
- No dependencies beyond C++17 standard library
