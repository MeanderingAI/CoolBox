// Minimal thread pool implementation
#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : stop_flag_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this, i] {
                try {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex_);
                            condition_.wait(lock, [this] { return stop_flag_ || !tasks_.empty(); });
                            if (stop_flag_ && tasks_.empty()) return;
                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }
                        task();
                    }
                } catch (const std::exception& ex) {
                    std::cerr << "[ThreadPool] Worker " << i << " exception: " << ex.what() << std::endl;
                } catch (...) {
                    std::cerr << "[ThreadPool] Worker " << i << " unknown exception." << std::endl;
                }
            });
        }
    }
    ~ThreadPool() {
        std::cout << "[ThreadPool] Destructor called." << std::endl;
        stop();
        std::cout << "[ThreadPool] All threads joined." << std::endl;
    }
    void enqueue(std::function<void()> f) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.push(std::move(f));
        }
        condition_.notify_one();
    }
    void stop() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_flag_ = true;
        }
        condition_.notify_all();
        for (std::thread &worker : workers_) {
            if (worker.joinable()) worker.join();
        }
        workers_.clear();
    }
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_flag_;
};
