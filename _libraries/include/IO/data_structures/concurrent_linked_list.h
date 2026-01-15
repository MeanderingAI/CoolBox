
#ifndef DATA_STRUCTURES_CONCURRENT_LINKED_LIST_H
#define DATA_STRUCTURES_CONCURRENT_LINKED_LIST_H

#include <memory>
#include <mutex>
#include <atomic>
#include <type_traits>
#include <cstddef>


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

    void push_front(const T& value) {
        auto new_node = std::make_shared<Node>(value);
        std::lock_guard<std::mutex> lock(head_mutex_);
        new_node->next = head_;
        head_ = new_node;
        size_++;
    }

    void push_back(const T& value) {
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

    bool pop_front(T& value) {
        std::lock_guard<std::mutex> lock(head_mutex_);
        if (!head_) return false;
        value = head_->data;
        head_ = head_->next;
        size_--;
        return true;
    }

    bool remove_value(const T& value) {
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

    bool find(const T& value) const {
        std::lock_guard<std::mutex> lock(head_mutex_);
        auto current = head_;
        while (current) {
            if (current->data == value) return true;
            current = current->next;
        }
        return false;
    }

    size_t size() const {
        return size_.load();
    }

    bool empty() const {
        return size_.load() == 0;
    }

private:
    std::shared_ptr<Node> head_;
    std::atomic<size_t> size_;
    mutable std::mutex head_mutex_;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_CONCURRENT_LINKED_LIST_H
