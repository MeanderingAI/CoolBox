#ifndef DATA_STRUCTURES_HASH_MAP_H
#define DATA_STRUCTURES_HASH_MAP_H

#include <vector>
#include <list>
#include <functional>
#include <stdexcept>
#include <utility>

namespace data_structures {

template<typename K, typename V>
class HashMap {
public:
    using KeyValue = std::pair<K, V>;
    
    explicit HashMap(size_t initial_capacity = 16, float load_factor = 0.75f)
        : capacity_(initial_capacity), load_factor_(load_factor), size_(0) {
        buckets_.resize(capacity_);
    }
    
    void insert(const K& key, const V& value);
    bool remove(const K& key);
    
    bool get(const K& key, V& value) const;
    V& operator[](const K& key);
    
    bool contains(const K& key) const;
    
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear();
    
    std::vector<K> keys() const;
    std::vector<V> values() const;
    
private:
    std::vector<std::list<KeyValue>> buckets_;
    size_t capacity_;
    size_t size_;
    float load_factor_;
    std::hash<K> hasher_;
    
    size_t hash(const K& key) const;
    void rehash();
};

} // namespace data_structures

#endif // DATA_STRUCTURES_HASH_MAP_H
