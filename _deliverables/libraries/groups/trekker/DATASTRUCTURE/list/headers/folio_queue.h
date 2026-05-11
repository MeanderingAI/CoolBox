// folio_queue.h
// Folio Queue implementation for the DATASTRUCTURE package
// Author: [Your Name]
// License: Same as project
#pragma once


#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <algorithm>
#include <vector>

// A thread-safe FIFO queue with folio (batch) dequeue support
// A "folio" is a batch of items dequeued at once

template <typename T>
class folio_queue {
public:
    folio_queue() = default;

    // Enqueue a single item
    void enqueue(const T& item) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push_back(item);
        cv_.notify_one();
    }

    // Enqueue a batch of items
    void enqueue_batch(const std::vector<T>& items) {
        std::lock_guard<std::mutex> lock(mtx_);
        for (const auto& item : items) queue_.push_back(item);
        cv_.notify_all();
    }

    // Dequeue a single item (blocking)
    T dequeue() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&]{ return !queue_.empty(); });
        T item = queue_.front();
        queue_.pop_front();
        return item;
    }

    // Dequeue a folio (batch) of up to max_items (blocking until at least one is available)
    std::vector<T> dequeue_folio(size_t max_items) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&]{ return !queue_.empty(); });
        std::vector<T> folio;
        size_t n = std::min(max_items, queue_.size());
        for (size_t i = 0; i < n; ++i) {
            folio.push_back(queue_.front());
            queue_.pop_front();
        }
        return folio;
    }

    // Try to dequeue a single item (non-blocking)
    std::optional<T> try_dequeue() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) return std::nullopt;
        T item = queue_.front();
        queue_.pop_front();
        return item;
    }

    // Check if the queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

    // Get the current size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<T> queue_;
};
