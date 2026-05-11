#ifndef DISTRIBUTED_TASK_QUEUE_H
#define DISTRIBUTED_TASK_QUEUE_H

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>

namespace distributed {

// Thread-safe, bounded-or-unbounded task queue for distributing work across
// concurrent consumers. Supports blocking dequeue with timeout and graceful
// close semantics. max_size == 0 means unbounded.
template <typename Task>
class TaskQueue {
public:
    explicit TaskQueue(std::size_t max_size = 0)
        : max_size_(max_size), closed_(false) {}

    ~TaskQueue() { close(); }

    TaskQueue(const TaskQueue&)            = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    // Enqueue a task. Blocks when the queue is at capacity (bounded mode).
    // Returns false if the queue is closed.
    bool enqueue(Task task) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (max_size_ > 0) {
            not_full_.wait(lock, [this] {
                return closed_ || queue_.size() < max_size_;
            });
        }
        if (closed_) return false;
        queue_.push(std::move(task));
        not_empty_.notify_one();
        return true;
    }

    // Non-blocking enqueue. Returns false if closed or at capacity.
    bool try_enqueue(Task task) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) return false;
        if (max_size_ > 0 && queue_.size() >= max_size_) return false;
        queue_.push(std::move(task));
        not_empty_.notify_one();
        return true;
    }

    // Blocking dequeue. Returns nullopt when queue is closed and empty.
    std::optional<Task> dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return !queue_.empty() || closed_; });
        if (queue_.empty()) return std::nullopt;
        return pop_front(lock);
    }

    // Blocking dequeue with timeout. Returns nullopt on timeout or closed+empty.
    std::optional<Task> dequeue_for(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!not_empty_.wait_for(lock, timeout, [this] {
                return !queue_.empty() || closed_;
            })) {
            return std::nullopt;
        }
        if (queue_.empty()) return std::nullopt;
        return pop_front(lock);
    }

    // Non-blocking dequeue. Returns nullopt immediately if empty.
    std::optional<Task> try_dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        return pop_front(lock);
    }

    // Signal all waiting threads to wake and stop blocking.
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    bool        empty()     const { std::lock_guard<std::mutex> l(mutex_); return queue_.empty(); }
    bool        is_closed() const { std::lock_guard<std::mutex> l(mutex_); return closed_; }
    std::size_t size()      const { std::lock_guard<std::mutex> l(mutex_); return queue_.size(); }

private:
    std::optional<Task> pop_front(std::unique_lock<std::mutex>&) {
        Task t = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return t;
    }

    std::queue<Task>        queue_;
    mutable std::mutex      mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::size_t             max_size_;
    bool                    closed_;
};

} // namespace distributed

#endif // DISTRIBUTED_TASK_QUEUE_H
