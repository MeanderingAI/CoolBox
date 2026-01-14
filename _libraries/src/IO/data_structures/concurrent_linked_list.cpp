#include "IO/data_structures/concurrent_linked_list.h"

namespace data_structures {

template<typename T>
void ConcurrentLinkedList<T>::push_front(const T& value) {
    auto new_node = std::make_shared<Node>(value);
    
    std::lock_guard<std::mutex> lock(head_mutex_);
    new_node->next = head_;
    head_ = new_node;
    size_++;
}

template<typename T>
void ConcurrentLinkedList<T>::push_back(const T& value) {
    auto new_node = std::make_shared<Node>(value);
    
    std::lock_guard<std::mutex> lock(head_mutex_);
    
    if (!head_) {
        head_ = new_node;
    } else {
        auto current = head_;
        while (current->next) {
            current = current->next;
        }
        current->next = new_node;
    }
    
    size_++;
}

template<typename T>
bool ConcurrentLinkedList<T>::pop_front(T& value) {
    std::lock_guard<std::mutex> lock(head_mutex_);
    
    if (!head_) return false;
    
    value = head_->data;
    head_ = head_->next;
    size_--;
    
    return true;
}

template<typename T>
bool ConcurrentLinkedList<T>::remove_value(const T& value) {
    std::lock_guard<std::mutex> lock(head_mutex_);
    
    if (!head_) return false;
    
    if (head_->data == value) {
        head_ = head_->next;
        size_--;
        return true;
    }
    
    auto current = head_;
    while (current->next && current->next->data != value) {
        current = current->next;
    }
    
    if (!current->next) return false;
    
    current->next = current->next->next;
    size_--;
    return true;
}

template<typename T>
bool ConcurrentLinkedList<T>::find(const T& value) const {
    std::lock_guard<std::mutex> lock(head_mutex_);
    
    auto current = head_;
    while (current) {
        if (current->data == value) return true;
        current = current->next;
    }
    
    return false;
}

template<typename T>
size_t ConcurrentLinkedList<T>::size() const {
    return size_.load();
}

template<typename T>
bool ConcurrentLinkedList<T>::empty() const {
    return size_.load() == 0;
}

// Explicit template instantiations
template class ConcurrentLinkedList<int>;
template class ConcurrentLinkedList<double>;
template class ConcurrentLinkedList<std::string>;

} // namespace data_structures
