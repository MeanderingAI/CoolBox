#include "data_structures/hash_map.h"

// Library metadata
extern "C" {
    __attribute__((visibility("default"), used))
    const char* get_library_name() { return "data_structures"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_version() { return "1.0.0"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_description() { return "Core data structures: hash maps, linked lists, binary search trees, and concurrent variants"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_author() { return "ToolBox Team"; }
}

namespace data_structures {

template<typename K, typename V>
void HashMap<K, V>::insert(const K& key, const V& value) {
    size_t index = hash(key);
    auto& bucket = buckets_[index];
    
    // Check if key exists and update
    for (auto& kv : bucket) {
        if (kv.first == key) {
            kv.second = value;
            return;
        }
    }
    
    // Insert new key-value pair
    bucket.push_back({key, value});
    size_++;
    
    // Check load factor and rehash if necessary
    if (static_cast<float>(size_) / capacity_ > load_factor_) {
        rehash();
    }
}

template<typename K, typename V>
bool HashMap<K, V>::remove(const K& key) {
    size_t index = hash(key);
    auto& bucket = buckets_[index];
    
    for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if (it->first == key) {
            bucket.erase(it);
            size_--;
            return true;
        }
    }
    
    return false;
}

template<typename K, typename V>
bool HashMap<K, V>::get(const K& key, V& value) const {
    size_t index = hash(key);
    const auto& bucket = buckets_[index];
    
    for (const auto& kv : bucket) {
        if (kv.first == key) {
            value = kv.second;
            return true;
        }
    }
    
    return false;
}

template<typename K, typename V>
V& HashMap<K, V>::operator[](const K& key) {
    size_t index = hash(key);
    auto& bucket = buckets_[index];
    
    for (auto& kv : bucket) {
        if (kv.first == key) {
            return kv.second;
        }
    }
    
    // Key doesn't exist, insert with default value
    bucket.push_back({key, V()});
    size_++;
    
    if (static_cast<float>(size_) / capacity_ > load_factor_) {
        rehash();
        // Re-find after rehash
        index = hash(key);
        auto& new_bucket = buckets_[index];
        for (auto& kv : new_bucket) {
            if (kv.first == key) {
                return kv.second;
            }
        }
    }
    
    return bucket.back().second;
}

template<typename K, typename V>
bool HashMap<K, V>::contains(const K& key) const {
    size_t index = hash(key);
    const auto& bucket = buckets_[index];
    
    for (const auto& kv : bucket) {
        if (kv.first == key) {
            return true;
        }
    }
    
    return false;
}

template<typename K, typename V>
void HashMap<K, V>::clear() {
    for (auto& bucket : buckets_) {
        bucket.clear();
    }
    size_ = 0;
}

template<typename K, typename V>
std::vector<K> HashMap<K, V>::keys() const {
    std::vector<K> result;
    result.reserve(size_);
    
    for (const auto& bucket : buckets_) {
        for (const auto& kv : bucket) {
            result.push_back(kv.first);
        }
    }
    
    return result;
}

template<typename K, typename V>
std::vector<V> HashMap<K, V>::values() const {
    std::vector<V> result;
    result.reserve(size_);
    
    for (const auto& bucket : buckets_) {
        for (const auto& kv : bucket) {
            result.push_back(kv.second);
        }
    }
    
    return result;
}

template<typename K, typename V>
size_t HashMap<K, V>::hash(const K& key) const {
    return hasher_(key) % capacity_;
}

template<typename K, typename V>
void HashMap<K, V>::rehash() {
    size_t new_capacity = capacity_ * 2;
    std::vector<std::list<KeyValue>> new_buckets(new_capacity);
    
    // Rehash all existing elements
    for (auto& bucket : buckets_) {
        for (auto& kv : bucket) {
            size_t new_index = hasher_(kv.first) % new_capacity;
            new_buckets[new_index].push_back(std::move(kv));
        }
    }
    
    buckets_ = std::move(new_buckets);
    capacity_ = new_capacity;
}

// Explicit template instantiations
template class HashMap<int, int>;
template class HashMap<int, double>;
template class HashMap<int, std::string>;
template class HashMap<std::string, int>;
template class HashMap<std::string, double>;
template class HashMap<std::string, std::string>;

} // namespace data_structures
