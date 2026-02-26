#include <iostream>
#include <thread>
#include <chrono>
#include "services/cache_server/cache_server.h"

using namespace services;

void demo_basic_operations(DistributedCache& cache) {
    std::cout << "\n=== Basic String Operations ===\n";
    
    // SET and GET
    cache.set("user:1:name", "Alice");
    cache.set("user:1:age", "25");
    
    auto name = cache.get("user:1:name");
    auto age = cache.get("user:1:age");
    
    std::cout << "Name: " << (name ? *name : "null") << "\n";
    std::cout << "Age: " << (age ? *age : "null") << "\n";
    
    // Increment operations
    auto counter = cache.incr("page:views");
    std::cout << "Page views: " << (counter ? *counter : -1) << "\n";
    
    cache.incrby("page:views", 10);
    auto page_views = cache.get("page:views");
    std::cout << "Page views after +10: " << (page_views ? *page_views : "null") << "\n";
}

void demo_list_operations(DistributedCache& cache) {
    std::cout << "\n=== List Operations ===\n";
    
    // Task queue
    cache.rpush("tasks", "task1");
    cache.rpush("tasks", "task2");
    cache.rpush("tasks", "task3");
    cache.lpush("tasks", "urgent_task");
    
    std::cout << "Queue length: " << cache.llen("tasks") << "\n";
    
    // Process tasks
    auto task = cache.lpop("tasks");
    std::cout << "Processing: " << (task ? *task : "null") << "\n";
    
    task = cache.lpop("tasks");
    std::cout << "Processing: " << (task ? *task : "null") << "\n";
    
    std::cout << "Remaining tasks: " << cache.llen("tasks") << "\n";
}

void demo_set_operations(DistributedCache& cache) {
    std::cout << "\n=== Set Operations ===\n";
    
    // Unique visitors
    cache.sadd("visitors:today", "user1");
    cache.sadd("visitors:today", "user2");
    cache.sadd("visitors:today", "user3");
    cache.sadd("visitors:today", "user1"); // Duplicate, won't be added
    
    std::cout << "Unique visitors: " << cache.scard("visitors:today") << "\n";
    std::cout << "Is user2 a visitor? " << cache.sismember("visitors:today", "user2") << "\n";
    std::cout << "Is user99 a visitor? " << cache.sismember("visitors:today", "user99") << "\n";
    
    auto members = cache.smembers("visitors:today");
    std::cout << "All visitors: ";
    for (const auto& member : members) {
        std::cout << member << " ";
    }
    std::cout << "\n";
}

void demo_expiration(DistributedCache& cache) {
    std::cout << "\n=== Expiration (TTL) Operations ===\n";
    
    // Set key with 2 second TTL
    cache.set("session:abc123", "user_data", 2);
    
    auto session = cache.get("session:abc123");
    std::cout << "Session immediately: " << (session ? *session : "null") << "\n";
    
    auto ttl = cache.ttl("session:abc123");
    std::cout << "TTL: " << (ttl ? *ttl : -999) << " seconds\n";
    
    // Wait 3 seconds
    std::cout << "Waiting 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    session = cache.get("session:abc123");
    std::cout << "Session after expiration: " << (session ? *session : "null (expired)") << "\n";
}

void demo_concurrent_access(DistributedCache& cache) {
    std::cout << "\n=== Concurrent Access Demo ===\n";
    
    cache.set("counter", "0");
    
    auto increment_worker = [&cache](int id, int count) {
        for (int i = 0; i < count; ++i) {
            cache.incr("counter");
        }
    };
    
    const int num_threads = 10;
    const int increments_per_thread = 100;
    
    std::vector<std::thread> threads;
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(increment_worker, i, increments_per_thread);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    auto final_count = cache.get("counter");
    std::cout << "Final counter value: " << (final_count ? *final_count : "null") << "\n";
    std::cout << "Expected: " << (num_threads * increments_per_thread) << "\n";
    std::cout << "Time taken: " << duration.count() << "ms\n";
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "  Distributed Cache Service Demo (Redis-like)\n";
    std::cout << "  Using concurrent data structures\n";
    std::cout << "=================================================\n";
    
    // Create cache instance
    DistributedCache cache;
    
    // Run demos
    demo_basic_operations(cache);
    demo_list_operations(cache);
    demo_set_operations(cache);
    demo_expiration(cache);
    demo_concurrent_access(cache);
    
    // Show cache stats
    std::cout << "\n=== Cache Statistics ===\n";
    std::cout << "Total keys: " << cache.dbsize() << "\n";
    std::cout << "All keys: ";
    auto all_keys = cache.keys();
    for (const auto& key : all_keys) {
        std::cout << key << " ";
    }
    std::cout << "\n";
    
    // Optional: Start server for network access
    std::cout << "\n=== Starting Cache Server ===\n";
    std::cout << "To start the network server, uncomment the code below.\n";
    std::cout << "Then you can connect using: telnet localhost 6379\n";
    
    /*
    CacheServer server(6379);
    if (server.start()) {
        std::cout << "Cache server started on port 6379\n";
        std::cout << "Press Enter to stop...\n";
        std::cin.get();
        server.stop();
        std::cout << "Server stopped\n";
    } else {
        std::cerr << "Failed to start server\n";
    }
    */
    
    return 0;
}
