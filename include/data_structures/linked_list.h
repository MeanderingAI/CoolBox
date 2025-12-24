#ifndef DATA_STRUCTURES_LINKED_LIST_H
#define DATA_STRUCTURES_LINKED_LIST_H

#include <memory>
#include <functional>
#include <stdexcept>

namespace data_structures {

template<typename T>
class LinkedList {
public:
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        
        explicit Node(const T& value) : data(value), next(nullptr) {}
    };
    
    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}
    
    void push_front(const T& value);
    void push_back(const T& value);
    void insert_at(size_t index, const T& value);
    
    bool pop_front();
    bool pop_back();
    bool remove_at(size_t index);
    bool remove_value(const T& value);
    
    T front() const;
    T back() const;
    T at(size_t index) const;
    
    bool find(const T& value) const;
    void for_each(std::function<void(const T&)> callback) const;
    
    size_t size() const { return size_; }
    bool empty() const { return head_ == nullptr; }
    void clear();
    
    void reverse();
    
private:
    std::shared_ptr<Node> head_;
    std::shared_ptr<Node> tail_;
    size_t size_;
};

// Doubly Linked List
template<typename T>
class DoublyLinkedList {
public:
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        std::shared_ptr<Node> prev;
        
        explicit Node(const T& value) : data(value), next(nullptr), prev(nullptr) {}
    };
    
    DoublyLinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}
    
    void push_front(const T& value);
    void push_back(const T& value);
    void insert_at(size_t index, const T& value);
    
    bool pop_front();
    bool pop_back();
    bool remove_at(size_t index);
    
    T front() const;
    T back() const;
    
    size_t size() const { return size_; }
    bool empty() const { return head_ == nullptr; }
    void clear();
    
private:
    std::shared_ptr<Node> head_;
    std::shared_ptr<Node> tail_;
    size_t size_;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_LINKED_LIST_H
