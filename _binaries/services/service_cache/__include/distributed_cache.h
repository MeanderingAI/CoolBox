
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <chrono>
#include "IO/data_structures/concurrent_hash_map.h"
#include "IO/data_structures/concurrent_linked_list.h"

namespace services {

// Entry with expiration support
template<typename T>
struct CacheEntry {
    T value;
    std::chrono::steady_clock::time_point expiration;
    bool has_expiration;
    
    CacheEntry(const T& val) 
        : value(val), has_expiration(false) {}
    
    CacheEntry(const T& val, std::chrono::steady_clock::time_point exp)
        : value(val), expiration(exp), has_expiration(true) {}
    
    bool is_expired() const {
        if (!has_expiration) return false;
        return std::chrono::steady_clock::now() > expiration;
    }
};

// Distributed Cache Service (Redis-like)
class DistributedCache {
public:
    DistributedCache();
    ~DistributedCache();
    
    // String operations
    bool set(const std::string& key, const std::string& value);
    bool set(const std::string& key, const std::string& value, int ttl_seconds);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    
    // List operations
    bool lpush(const std::string& key, const std::string& value);
    bool rpush(const std::string& key, const std::string& value);
    std::optional<std::string> lpop(const std::string& key);
    std::optional<std::string> rpop(const std::string& key);
    size_t llen(const std::string& key);
    std::vector<std::string> lrange(const std::string& key, int start, int stop);
    
    // Set operations
    bool sadd(const std::string& key, const std::string& member);
    bool srem(const std::string& key, const std::string& member);
    bool sismember(const std::string& key, const std::string& member);
    size_t scard(const std::string& key);
    std::vector<std::string> smembers(const std::string& key);
    
    // General operations
    std::vector<std::string> keys();
    bool flush();
    size_t dbsize();
    
    // Increment/Decrement operations
    std::optional<int64_t> incr(const std::string& key);
    std::optional<int64_t> decr(const std::string& key);
    std::optional<int64_t> incrby(const std::string& key, int64_t increment);
    
    // TTL operations
    bool expire(const std::string& key, int seconds);
    std::optional<int> ttl(const std::string& key);
    bool persist(const std::string& key);
    
private:
    // Storage backends using concurrent data structures
    std::unique_ptr<data_structures::ConcurrentHashMap<std::string, 
                    std::shared_ptr<CacheEntry<std::string>>>> string_store_;
    std::unique_ptr<data_structures::ConcurrentHashMap<std::string, 
                    std::shared_ptr<data_structures::ConcurrentLinkedList<std::string>>>> list_store_;
    std::unique_ptr<data_structures::ConcurrentHashMap<std::string, 
                    std::shared_ptr<data_structures::ConcurrentHashMap<std::string, bool>>>> set_store_;
    
    // Helper methods
    void cleanup_expired();
    bool is_expired(const std::string& key);
};

} // namespace services
