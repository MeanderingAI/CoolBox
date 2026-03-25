#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <functional>
#include <cstddef>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t num_threads);
    ~ThreadPool();

    // Enqueue a task to be executed by the pool
    void enqueue(std::function<void()> f);

    // Stop the pool and join worker threads
    void stop();

private:
    // PIMPL-like minimal private members defined in source file
    struct Impl;
    Impl* impl_;
};

#endif // THREAD_POOL_H
