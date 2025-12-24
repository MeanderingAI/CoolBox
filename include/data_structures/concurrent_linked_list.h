#ifndef DATA_STRUCTURES_CONCURRENT_LINKED_LIST_H
#define DATA_STRUCTURES_CONCURRENT_LINKED_LIST_H

#include <memory>
#include <mutex>
#include <atomic>

namespace data_structures {

// Lock-based concurrent linked list
template<typename T>
class ConcurrentLinkedList {
public:
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        std::unique_ptr<std::mutex> node_mutex;
        
        explicit Node(const T& value) 
            : data(value), next(nullptr), node_mutex(std::make_unique<std::mutex>()) {}
    };
    
    ConcurrentLinkedList() : head_(nullptr), size_(0) {}
    
    void push_front(const T& value);
    void push_back(const T& value);
    
    bool pop_front(T& value);
    bool remove_value(const T& value);
    
    bool find(const T& value) const;
    
    size_t size() const;
    bool empty() const;
    
private:
    std::shared_ptr<Node> head_;
    std::atomic<size_t> size_;
    mutable std::mutex head_mutex_;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_CONCURRENT_LINKED_LIST_H
