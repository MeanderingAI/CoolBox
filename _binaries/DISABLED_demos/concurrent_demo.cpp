#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <chrono>
#include "data_structures/concurrent_hash_map.h"
#include "data_structures/concurrent_linked_list.h"

using namespace data_structures;

void demo_concurrent_hash_map() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Concurrent HashMap Demo             ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ConcurrentHashMap<std::string, int> map;
    
    // Single-threaded operations
    std::cout << "Single-threaded operations:\n";
    map.insert("Alice", 25);
    map.insert("Bob", 30);
    map.insert("Charlie", 35);
    
    int age;
    if (map.get("Alice", age)) {
        std::cout << "  Alice's age: " << age << "\n";
    }
    
    std::cout << "  Map size: " << map.size() << "\n";
    
    // Multi-threaded insertions
    std::cout << "\nMulti-threaded insertions:\n";
    const int num_threads = 4;
    const int items_per_thread = 250;
    
    std::vector<std::thread> threads;
    auto start = std::chrono::steady_clock::now();
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&map, t, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                int key = t * items_per_thread + i;
                map.insert("key_" + std::to_string(key), key);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Inserted " << (num_threads * items_per_thread) 
              << " items from " << num_threads << " threads\n";
    std::cout << "  Final size: " << map.size() << "\n";
    std::cout << "  Time taken: " << duration.count() << "ms\n";
}

void demo_concurrent_linked_list() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Concurrent LinkedList Demo          ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ConcurrentLinkedList<int> list;
    
    // Single-threaded operations
    std::cout << "Single-threaded operations:\n";
    list.push_back(10);
    list.push_back(20);
    list.push_back(30);
    list.push_front(5);
    
    std::cout << "  List size: " << list.size() << "\n";
    
    int value;
    if (list.pop_front(value)) {
        std::cout << "  Popped value: " << value << "\n";
    }
    
    std::cout << "  Contains 20: " << (list.find(20) ? "Yes" : "No") << "\n";
    std::cout << "  Contains 100: " << (list.find(100) ? "Yes" : "No") << "\n";
}

void demo_producer_consumer() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Producer-Consumer Pattern           ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ConcurrentLinkedList<int> queue;
    std::atomic<int> produced(0);
    std::atomic<int> consumed(0);
    const int total_items = 1000;
    
    std::cout << "Starting producer-consumer demo with " << total_items << " items...\n";
    
    auto start = std::chrono::steady_clock::now();
    
    // Producer threads
    std::thread producer1([&queue, &produced, total_items]() {
        for (int i = 0; i < total_items / 2; ++i) {
            queue.push_back(i);
            produced++;
        }
    });
    
    std::thread producer2([&queue, &produced, total_items]() {
        for (int i = total_items / 2; i < total_items; ++i) {
            queue.push_back(i);
            produced++;
        }
    });
    
    // Consumer threads
    std::thread consumer1([&queue, &consumed, total_items]() {
        int value;
        while (consumed < total_items) {
            if (queue.pop_front(value)) {
                consumed++;
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    });
    
    producer1.join();
    producer2.join();
    consumer1.join();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Produced: " << produced.load() << " items\n";
    std::cout << "  Consumed: " << consumed.load() << " items\n";
    std::cout << "  Queue size: " << queue.size() << " (should be 0)\n";
    std::cout << "  Time taken: " << duration.count() << "ms\n";
}

void demo_concurrent_reads_writes() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Concurrent Reads & Writes           ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ConcurrentHashMap<int, int> map;
    
    // Pre-populate
    std::cout << "Pre-populating map with 100 entries...\n";
    for (int i = 0; i < 100; ++i) {
        map.insert(i, i * 10);
    }
    
    std::atomic<int> reads_done(0);
    std::atomic<int> writes_done(0);
    
    auto start = std::chrono::steady_clock::now();
    
    // Writer threads
    std::vector<std::thread> threads;
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&map, &writes_done, t]() {
            for (int i = 0; i < 500; ++i) {
                map.insert(100 + t * 500 + i, i);
                writes_done++;
            }
        });
    }
    
    // Reader threads
    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&map, &reads_done]() {
            for (int i = 0; i < 1000; ++i) {
                int value;
                map.get(i % 100, value);
                reads_done++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Reads performed: " << reads_done.load() << "\n";
    std::cout << "  Writes performed: " << writes_done.load() << "\n";
    std::cout << "  Final map size: " << map.size() << "\n";
    std::cout << "  Time taken: " << duration.count() << "ms\n";
}

void demo_stress_test() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Stress Test                         ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    ConcurrentHashMap<int, int> map;
    const int num_threads = 8;
    const int ops_per_thread = 5000;
    
    std::cout << "Running stress test with " << num_threads 
              << " threads, " << ops_per_thread << " operations each...\n";
    
    std::atomic<int> ops_completed(0);
    auto start = std::chrono::steady_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&map, &ops_completed, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                int key = (t * ops_per_thread + i) % 1000;
                
                // Mix of operations
                if (i % 3 == 0) {
                    map.insert(key, i);
                } else if (i % 3 == 1) {
                    int value;
                    map.get(key, value);
                } else {
                    map.remove(key);
                }
                ops_completed++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Total operations: " << ops_completed.load() << "\n";
    std::cout << "  Final map size: " << map.size() << "\n";
    std::cout << "  Time taken: " << duration.count() << "ms\n";
    std::cout << "  Throughput: " << (ops_completed.load() / (duration.count() / 1000.0)) 
              << " ops/sec\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║    Concurrent Data Structures Demo                ║\n";
    std::cout << "║    Thread-Safe Operations                         ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    demo_concurrent_hash_map();
    demo_concurrent_linked_list();
    demo_producer_consumer();
    demo_concurrent_reads_writes();
    demo_stress_test();
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Demo Complete!                      ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    return 0;
}
