#include "IO/data_structures/linked_list.h"

namespace data_structures {

// LinkedList implementation
template<typename T>
void LinkedList<T>::push_front(const T& value) {
    auto new_node = std::make_shared<Node>(value);
    new_node->next = head_;
    head_ = new_node;
    
    if (!tail_) {
        tail_ = head_;
    }
    
    size_++;
}

template<typename T>
void LinkedList<T>::push_back(const T& value) {
    auto new_node = std::make_shared<Node>(value);
    
    if (!head_) {
        head_ = tail_ = new_node;
    } else {
        tail_->next = new_node;
        tail_ = new_node;
    }
    
    size_++;
}

template<typename T>
void LinkedList<T>::insert_at(size_t index, const T& value) {
    if (index > size_) {
        throw std::out_of_range("Index out of range");
    }
    
    if (index == 0) {
        push_front(value);
        return;
    }
    
    if (index == size_) {
        push_back(value);
        return;
    }
    
    auto current = head_;
    for (size_t i = 0; i < index - 1; ++i) {
        current = current->next;
    }
    
    auto new_node = std::make_shared<Node>(value);
    new_node->next = current->next;
    current->next = new_node;
    size_++;
}

template<typename T>
bool LinkedList<T>::pop_front() {
    if (!head_) return false;
    
    head_ = head_->next;
    if (!head_) {
        tail_ = nullptr;
    }
    
    size_--;
    return true;
}

template<typename T>
bool LinkedList<T>::pop_back() {
    if (!head_) return false;
    
    if (head_ == tail_) {
        head_ = tail_ = nullptr;
        size_--;
        return true;
    }
    
    auto current = head_;
    while (current->next != tail_) {
        current = current->next;
    }
    
    current->next = nullptr;
    tail_ = current;
    size_--;
    return true;
}

template<typename T>
bool LinkedList<T>::remove_at(size_t index) {
    if (index >= size_) return false;
    
    if (index == 0) {
        return pop_front();
    }
    
    auto current = head_;
    for (size_t i = 0; i < index - 1; ++i) {
        current = current->next;
    }
    
    auto to_remove = current->next;
    current->next = to_remove->next;
    
    if (to_remove == tail_) {
        tail_ = current;
    }
    
    size_--;
    return true;
}

template<typename T>
bool LinkedList<T>::remove_value(const T& value) {
    if (!head_) return false;
    
    if (head_->data == value) {
        return pop_front();
    }
    
    auto current = head_;
    while (current->next && current->next->data != value) {
        current = current->next;
    }
    
    if (!current->next) return false;
    
    auto to_remove = current->next;
    current->next = to_remove->next;
    
    if (to_remove == tail_) {
        tail_ = current;
    }
    
    size_--;
    return true;
}

template<typename T>
T LinkedList<T>::front() const {
    if (!head_) throw std::runtime_error("List is empty");
    return head_->data;
}

template<typename T>
T LinkedList<T>::back() const {
    if (!tail_) throw std::runtime_error("List is empty");
    return tail_->data;
}

template<typename T>
T LinkedList<T>::at(size_t index) const {
    if (index >= size_) throw std::out_of_range("Index out of range");
    
    auto current = head_;
    for (size_t i = 0; i < index; ++i) {
        current = current->next;
    }
    
    return current->data;
}

template<typename T>
bool LinkedList<T>::find(const T& value) const {
    auto current = head_;
    while (current) {
        if (current->data == value) return true;
        current = current->next;
    }
    return false;
}

template<typename T>
void LinkedList<T>::for_each(std::function<void(const T&)> callback) const {
    auto current = head_;
    while (current) {
        callback(current->data);
        current = current->next;
    }
}

template<typename T>
void LinkedList<T>::clear() {
    head_ = tail_ = nullptr;
    size_ = 0;
}

template<typename T>
void LinkedList<T>::reverse() {
    if (!head_ || !head_->next) return;
    
    std::shared_ptr<Node> prev = nullptr;
    auto current = head_;
    tail_ = head_;
    
    while (current) {
        auto next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    
    head_ = prev;
}

// DoublyLinkedList implementation
template<typename T>
void DoublyLinkedList<T>::push_front(const T& value) {
    auto new_node = std::make_shared<Node>(value);
    
    if (!head_) {
        head_ = tail_ = new_node;
    } else {
        new_node->next = head_;
        head_->prev = new_node;
        head_ = new_node;
    }
    
    size_++;
}

template<typename T>
void DoublyLinkedList<T>::push_back(const T& value) {
    auto new_node = std::make_shared<Node>(value);
    
    if (!tail_) {
        head_ = tail_ = new_node;
    } else {
        new_node->prev = tail_;
        tail_->next = new_node;
        tail_ = new_node;
    }
    
    size_++;
}

template<typename T>
bool DoublyLinkedList<T>::pop_front() {
    if (!head_) return false;
    
    head_ = head_->next;
    if (head_) {
        head_->prev = nullptr;
    } else {
        tail_ = nullptr;
    }
    
    size_--;
    return true;
}

template<typename T>
bool DoublyLinkedList<T>::pop_back() {
    if (!tail_) return false;
    
    tail_ = tail_->prev;
    if (tail_) {
        tail_->next = nullptr;
    } else {
        head_ = nullptr;
    }
    
    size_--;
    return true;
}

template<typename T>
T DoublyLinkedList<T>::front() const {
    if (!head_) throw std::runtime_error("List is empty");
    return head_->data;
}

template<typename T>
T DoublyLinkedList<T>::back() const {
    if (!tail_) throw std::runtime_error("List is empty");
    return tail_->data;
}

template<typename T>
void DoublyLinkedList<T>::clear() {
    head_ = tail_ = nullptr;
    size_ = 0;
}

// Explicit template instantiations
template class LinkedList<int>;
template class LinkedList<double>;
template class LinkedList<std::string>;
template class DoublyLinkedList<int>;
template class DoublyLinkedList<double>;
template class DoublyLinkedList<std::string>;

} // namespace data_structures
