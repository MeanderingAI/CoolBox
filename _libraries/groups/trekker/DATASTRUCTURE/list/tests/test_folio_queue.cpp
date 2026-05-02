// test_folio_queue.cpp
// Basic tests for folio_queue
#include "folio_queue.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

void test_single_thread() {
    folio_queue<int> fq;
    fq.enqueue(1);
    fq.enqueue(2);
    fq.enqueue(3);
    assert(fq.size() == 3);
    assert(fq.dequeue() == 1);
    assert(fq.dequeue() == 2);
    assert(fq.dequeue() == 3);
    assert(fq.empty());
}

void test_batch() {
    folio_queue<int> fq;
    fq.enqueue_batch({10, 20, 30, 40});
    auto folio = fq.dequeue_folio(3);
    assert(folio.size() == 3);
    assert(folio[0] == 10 && folio[1] == 20 && folio[2] == 30);
    assert(fq.size() == 1);
    assert(fq.dequeue() == 40);
}

void test_multithread() {
    folio_queue<int> fq;
    std::thread producer([&](){
        for (int i = 0; i < 5; ++i) fq.enqueue(i);
    });
    std::vector<int> results;
    std::thread consumer([&](){
        for (int i = 0; i < 5; ++i) results.push_back(fq.dequeue());
    });
    producer.join();
    consumer.join();
    for (int i = 0; i < 5; ++i) assert(results[i] == i);
}

int main() {
    test_single_thread();
    test_batch();
    test_multithread();
    std::cout << "All folio_queue tests passed!\n";
    return 0;
}
