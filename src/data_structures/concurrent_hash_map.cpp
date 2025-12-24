#include "data_structures/concurrent_hash_map.h"
#include "data_structures/concurrent_linked_list.h"

namespace data_structures {

template<typename K, typename V>
void ConcurrentHashMap<K, V>::insert(const K& key, const V& value) {
    size_t index = hash(key);
    auto& bucket = buckets_[index];
    
    {
        std::unique_lock<std::shared_mutex> lock(*bucket->mutex);
        
        // Check if key exists and update
        for (auto& kv : bucket->data) {
            if (kv.first == key) {
                kv.second = value;
                return;
            }
        }
        
        // Insert new key-value pair
        bucket->data.push_back({key, value});
    }
    
    std::unique_lock<std::shared_mutex> size_lock(size_mutex_);
    size_++;
}

template<typename K, typename V>
bool ConcurrentHashMap<K, V>::remove(const K& key) {
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

template<typename K, typename V>
bool ConcurrentHashMap<K, V>::get(const K& key, V& value) const {
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

template<typename K, typename V>
bool ConcurrentHashMap<K, V>::contains(const K& key) const {
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

template<typename K, typename V>
size_t ConcurrentHashMap<K, V>::size() const {
    std::shared_lock<std::shared_mutex> lock(size_mutex_);
    return size_;
}

template<typename K, typename V>
bool ConcurrentHashMap<K, V>::empty() const {
    return size() == 0;
}

template<typename K, typename V>
void ConcurrentHashMap<K, V>::clear() {
    for (auto& bucket : buckets_) {
        std::unique_lock<std::shared_mutex> lock(*bucket->mutex);
        bucket->data.clear();
    }
    
    std::unique_lock<std::shared_mutex> size_lock(size_mutex_);
    size_ = 0;
}

template<typename K, typename V>
std::vector<K> ConcurrentHashMap<K, V>::keys() const {
    std::vector<K> result;
    
    for (const auto& bucket : buckets_) {
        std::shared_lock<std::shared_mutex> lock(*bucket->mutex);
        for (const auto& kv : bucket->data) {
            result.push_back(kv.first);
        }
    }
    
    return result;
}

template<typename K, typename V>
size_t ConcurrentHashMap<K, V>::hash(const K& key) const {
    return hasher_(key) % capacity_;
}

// Explicit template instantiations
template class ConcurrentHashMap<int, int>;
template class ConcurrentHashMap<int, double>;
template class ConcurrentHashMap<int, std::string>;
template class ConcurrentHashMap<std::string, int>;
template class ConcurrentHashMap<std::string, double>;
template class ConcurrentHashMap<std::string, std::string>;
template class ConcurrentHashMap<std::string, bool>;

// For services library - using forward declarations to avoid circular dependency
template class ConcurrentHashMap<std::string, std::shared_ptr<ConcurrentHashMap<std::string, bool>>>;
template class ConcurrentHashMap<std::string, std::shared_ptr<ConcurrentLinkedList<std::string>>>;

} // namespace data_structures

// Additional instantiations for services (requires services headers)
#include "services/cache_server/distributed_cache.h"
#include "services/dns/dns_server.h"
#include "services/proxy/proxy_server.h"

namespace data_structures {
template class ConcurrentHashMap<std::string, std::shared_ptr<services::CacheEntry<std::string>>>;
template class ConcurrentHashMap<std::string, std::vector<services::dns::DNSRecord>>;
template class ConcurrentHashMap<std::string, std::shared_ptr<services::proxy::CachedResponse>>;

} // namespace data_structures
