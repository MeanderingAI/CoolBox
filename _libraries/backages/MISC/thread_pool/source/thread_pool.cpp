#include "thread_pool.h"

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <vector>
#include <memory>
#include <iostream>

struct ThreadPool::Impl {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop_flag{false};
};

ThreadPool::ThreadPool(std::size_t num_threads) : impl_(new Impl()) {
    for (std::size_t i = 0; i < num_threads; ++i) {
        impl_->workers.emplace_back([this, i] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->impl_->queue_mutex);
                    this->impl_->condition.wait(lock, [this] {
                        return this->impl_->stop_flag.load() || !this->impl_->tasks.empty();
                    });
                    if (this->impl_->stop_flag.load() && this->impl_->tasks.empty()) return;
                    task = std::move(this->impl_->tasks.front());
                    this->impl_->tasks.pop();
                }
                try {
                    task();
                } catch (const std::exception& ex) {
                    std::cerr << "[ThreadPool] Worker exception: " << ex.what() << std::endl;
                } catch (...) {
                    std::cerr << "[ThreadPool] Worker unknown exception." << std::endl;
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop();
    delete impl_;
}

void ThreadPool::enqueue(std::function<void()> f) {
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        impl_->tasks.push(std::move(f));
    }
    impl_->condition.notify_one();
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        impl_->stop_flag.store(true);
    }
    impl_->condition.notify_all();
    for (auto &w : impl_->workers) {
        if (w.joinable()) w.join();
    }
    impl_->workers.clear();
}
