
#ifndef DATA_STRUCTURES_CONCURRENT_HASH_MAP_H
#define DATA_STRUCTURES_CONCURRENT_HASH_MAP_H

#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <memory>
#include <type_traits>
#include <cstddef>

namespace data_structures {
template<typename K, typename V>
class ConcurrentHashMap {
public:
    using KeyValue = std::pair<K, V>;

    explicit ConcurrentHashMap(size_t initial_capacity = 16, float load_factor = 0.75f);

    void insert(const K& key, const V& value);
    bool remove(const K& key);
    bool get(const K& key, V& value) const;
    bool contains(const K& key) const;
    size_t size() const;
    bool empty() const;
    void clear();
    std::vector<K> keys() const;

private:
    struct Bucket {
        std::list<KeyValue> data;
    std::unique_ptr<std::shared_mutex> mutex;

    Bucket() : mutex(std::make_unique<std::shared_mutex>()) {}
    Bucket(const Bucket&) = delete;
    Bucket& operator=(const Bucket&) = delete;
        Bucket(Bucket&&) = default;
        Bucket& operator=(Bucket&&) = default;
    };
    std::vector<std::unique_ptr<Bucket>> buckets_;
    size_t capacity_;
    size_t size_;
    float load_factor_;
    std::hash<K> hasher_;
    mutable std::shared_mutex size_mutex_;
    size_t hash(const K& key) const;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_CONCURRENT_HASH_MAP_H
