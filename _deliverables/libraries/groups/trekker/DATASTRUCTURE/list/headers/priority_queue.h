#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace data_structures {

/**
 * BinaryHeap<T, Compare>
 *
 * A generic binary heap (priority queue) backed by a contiguous vector.
 *
 * Default Compare = std::less<T>  → max-heap  (largest element at top).
 * Use std::greater<T>             → min-heap  (smallest element at top).
 *
 * Complexity:
 *   push   — O(log n)
 *   pop    — O(log n)
 *   top    — O(1)
 */
template<typename T, typename Compare = std::less<T>>
class BinaryHeap {
public:
    explicit BinaryHeap(Compare cmp = Compare{}) : cmp_(std::move(cmp)) {}

    /** Insert a value into the heap. */
    void push(const T& value) {
        data_.push_back(value);
        sift_up(data_.size() - 1);
    }

    void push(T&& value) {
        data_.push_back(std::move(value));
        sift_up(data_.size() - 1);
    }

    /** Remove the top element. Throws std::underflow_error if empty. */
    void pop() {
        if (data_.empty()) throw std::underflow_error("BinaryHeap::pop on empty heap");
        std::swap(data_.front(), data_.back());
        data_.pop_back();
        if (!data_.empty()) sift_down(0);
    }

    /** Return a const reference to the top element. Throws if empty. */
    const T& top() const {
        if (data_.empty()) throw std::underflow_error("BinaryHeap::top on empty heap");
        return data_.front();
    }

    bool        empty() const { return data_.empty(); }
    std::size_t size()  const { return data_.size(); }

    /** Remove all elements. */
    void clear() { data_.clear(); }

private:
    std::vector<T> data_;
    Compare        cmp_;

    static std::size_t parent(std::size_t i) { return (i - 1) / 2; }
    static std::size_t left  (std::size_t i) { return 2 * i + 1;   }
    static std::size_t right (std::size_t i) { return 2 * i + 2;   }

    void sift_up(std::size_t i) {
        while (i > 0) {
            const std::size_t p = parent(i);
            if (cmp_(data_[p], data_[i])) {
                std::swap(data_[p], data_[i]);
                i = p;
            } else {
                break;
            }
        }
    }

    void sift_down(std::size_t i) {
        const std::size_t n = data_.size();
        while (true) {
            std::size_t best = i;
            const std::size_t l = left(i);
            const std::size_t r = right(i);
            if (l < n && cmp_(data_[best], data_[l])) best = l;
            if (r < n && cmp_(data_[best], data_[r])) best = r;
            if (best == i) break;
            std::swap(data_[i], data_[best]);
            i = best;
        }
    }
};


/**
 * IndexedPriorityQueue<Key, Priority, Compare>
 *
 * A binary heap that supports O(log n) priority updates and O(1) membership
 * queries, by maintaining a hash map from Key to heap position.
 *
 * Useful for algorithms such as Dijkstra's shortest path and Prim's MST.
 *
 * Key:      identifier type — must be hashable (usable as unordered_map key).
 * Priority: comparable type.
 *
 * Default Compare = std::greater<Priority>  → min-heap by priority.
 * Use std::less<Priority>                   → max-heap by priority.
 *
 * Complexity:
 *   push   — O(log n)  (no-op / update if key already present)
 *   pop    — O(log n)
 *   top    — O(1)
 *   update — O(log n)
 *   contains — O(1)
 */
template<typename Key,
         typename Priority,
         typename Compare = std::greater<Priority>>
class IndexedPriorityQueue {
public:
    explicit IndexedPriorityQueue(Compare cmp = Compare{}) : cmp_(std::move(cmp)) {}

    /**
     * Insert (key, priority). If key already exists the priority is updated
     * in-place; no duplicate keys are stored.
     */
    void push(const Key& key, const Priority& priority) {
        auto it = pos_.find(key);
        if (it != pos_.end()) {
            update(key, priority);
            return;
        }
        const std::size_t idx = heap_.size();
        heap_.push_back({key, priority});
        pos_[key] = idx;
        sift_up(idx);
    }

    /** Remove the top element. Throws std::underflow_error if empty. */
    void pop() {
        if (heap_.empty()) throw std::underflow_error("IndexedPriorityQueue::pop on empty queue");
        pos_.erase(heap_.front().key);
        swap_entries(0, heap_.size() - 1);
        heap_.pop_back();
        if (!heap_.empty()) sift_down(0);
    }

    /** Return (key, priority) of the top element. Throws if empty. */
    std::pair<Key, Priority> top() const {
        if (heap_.empty()) throw std::underflow_error("IndexedPriorityQueue::top on empty queue");
        return {heap_.front().key, heap_.front().priority};
    }

    /**
     * Update the priority of an existing key.
     * If key is not present it is inserted.
     */
    void update(const Key& key, const Priority& new_priority) {
        auto it = pos_.find(key);
        if (it == pos_.end()) {
            push(key, new_priority);
            return;
        }
        const std::size_t i    = it->second;
        const Priority    old  = heap_[i].priority;
        heap_[i].priority      = new_priority;
        // Determine which direction to restore heap order
        if (cmp_(old, new_priority)) {
            sift_up(i);
        } else {
            sift_down(i);
        }
    }

    /** Return true if key is currently in the queue. */
    bool contains(const Key& key) const {
        return pos_.count(key) > 0;
    }

    bool        empty() const { return heap_.empty(); }
    std::size_t size()  const { return heap_.size(); }

    /** Remove all elements. */
    void clear() { heap_.clear(); pos_.clear(); }

private:
    struct Entry {
        Key      key;
        Priority priority;
    };

    std::vector<Entry>                    heap_;
    std::unordered_map<Key, std::size_t>  pos_;   // key → index in heap_
    Compare                               cmp_;

    static std::size_t parent(std::size_t i) { return (i - 1) / 2; }
    static std::size_t left  (std::size_t i) { return 2 * i + 1;   }
    static std::size_t right (std::size_t i) { return 2 * i + 2;   }

    void swap_entries(std::size_t i, std::size_t j) {
        pos_[heap_[i].key] = j;
        pos_[heap_[j].key] = i;
        std::swap(heap_[i], heap_[j]);
    }

    void sift_up(std::size_t i) {
        while (i > 0) {
            const std::size_t p = parent(i);
            if (cmp_(heap_[p].priority, heap_[i].priority)) {
                swap_entries(p, i);
                i = p;
            } else {
                break;
            }
        }
    }

    void sift_down(std::size_t i) {
        const std::size_t n = heap_.size();
        while (true) {
            std::size_t best = i;
            const std::size_t l = left(i);
            const std::size_t r = right(i);
            if (l < n && cmp_(heap_[best].priority, heap_[l].priority)) best = l;
            if (r < n && cmp_(heap_[best].priority, heap_[r].priority)) best = r;
            if (best == i) break;
            swap_entries(i, best);
            i = best;
        }
    }
};

} // namespace data_structures
