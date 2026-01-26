
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

    explicit ConcurrentHashMap(size_t initial_capacity = 16, float load_factor = 0.75f) : capacity_(initial_capacity), size_(0), load_factor_(load_factor), buckets_(), hasher_(), size_mutex_() {
        buckets_.reserve(capacity_);
        for (size_t i = 0; i < capacity_; ++i) {
            buckets_.emplace_back(std::make_unique<Bucket>());
        }
    }

    void insert(const K& key, const V& value) {
        size_t index = hash(key);
        auto& bucket = buckets_[index];
        {
            std::unique_lock<std::shared_mutex> lock(*bucket->mutex);
            for (auto& kv : bucket->data) {
                if (kv.first == key) {
                    kv.second = value;
                    return;
                }
            }
            bucket->data.push_back({key, value});
        }
        std::unique_lock<std::shared_mutex> size_lock(size_mutex_);
        size_++;
    }

    bool remove(const K& key) {
        size_t index = hash(key);
        auto& bucket = buckets_[index];
        std::unique_lock<std::shared_mutex> lock(*bucket->mutex);
        for (auto it = bucket->data.begin(); it != bucket->data.end(); ++it) {
            if (it->first == key) {
                bucket->data.erase(it);
                std::unique_lock<std::shared_mutex> size_lock(size_mutex_);
                size_--;
                return true;
            }
        }
        return false;
    }

    bool get(const K& key, V& value) const {
        size_t index = hash(key);
        const auto& bucket = buckets_[index];
        std::shared_lock<std::shared_mutex> lock(*bucket->mutex);
        for (const auto& kv : bucket->data) {
            if (kv.first == key) {
                value = kv.second;
                return true;
            }
        }
        return false;
    }

    bool contains(const K& key) const {
        size_t index = hash(key);
        const auto& bucket = buckets_[index];
        std::shared_lock<std::shared_mutex> lock(*bucket->mutex);
        for (const auto& kv : bucket->data) {
            if (kv.first == key) {
                return true;
            }
        }
        return false;
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(size_mutex_);
        return size_;
    }

    bool empty() const {
        return size() == 0;
    }

    void clear() {
        for (auto& bucket : buckets_) {
            std::unique_lock<std::shared_mutex> lock(*bucket->mutex);
            bucket->data.clear();
        }
        std::unique_lock<std::shared_mutex> size_lock(size_mutex_);
        size_ = 0;
    }

    std::vector<K> keys() const {
        std::vector<K> result;
        for (const auto& bucket : buckets_) {
            std::shared_lock<std::shared_mutex> lock(*bucket->mutex);
            for (const auto& kv : bucket->data) {
                result.push_back(kv.first);
            }
        }
        return result;
    }

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
    size_t hash(const K& key) const {
        return hasher_(key) % capacity_;
    }
};

} // namespace data_structures

#endif // DATA_STRUCTURES_CONCURRENT_HASH_MAP_H
