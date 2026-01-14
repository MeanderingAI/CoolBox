#include "service_cache/__include/distributed_cache.h"
#include <sstream>
#include <algorithm>

namespace services {

DistributedCache::DistributedCache() 
    : string_store_(std::make_unique<data_structures::ConcurrentHashMap<std::string, 
                    std::shared_ptr<CacheEntry<std::string>>>>())
    , list_store_(std::make_unique<data_structures::ConcurrentHashMap<std::string, 
                  std::shared_ptr<data_structures::ConcurrentLinkedList<std::string>>>>())
    , set_store_(std::make_unique<data_structures::ConcurrentHashMap<std::string, 
                 std::shared_ptr<data_structures::ConcurrentHashMap<std::string, bool>>>>()) {
}

DistributedCache::~DistributedCache() = default;

// String operations
bool DistributedCache::set(const std::string& key, const std::string& value) {
    auto entry = std::make_shared<CacheEntry<std::string>>(value);
    string_store_->insert(key, entry);
    return true;
}

bool DistributedCache::set(const std::string& key, const std::string& value, int ttl_seconds) {
    auto expiration = std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
    auto entry = std::make_shared<CacheEntry<std::string>>(value, expiration);
    string_store_->insert(key, entry);
    return true;
}

std::optional<std::string> DistributedCache::get(const std::string& key) {
    std::shared_ptr<CacheEntry<std::string>> entry;
    if (!string_store_->get(key, entry)) {
        return std::nullopt;
    }
    
    if (entry->is_expired()) {
        string_store_->remove(key);
        return std::nullopt;
    }
    
    return entry->value;
}

bool DistributedCache::del(const std::string& key) {
    bool removed = string_store_->remove(key);
    removed |= list_store_->remove(key);
    removed |= set_store_->remove(key);
    return removed;
}

bool DistributedCache::exists(const std::string& key) {
    if (string_store_->contains(key)) {
        // Check if expired
        std::shared_ptr<CacheEntry<std::string>> entry;
        if (string_store_->get(key, entry) && !entry->is_expired()) {
            return true;
        }
        string_store_->remove(key);
    }
    return list_store_->contains(key) || set_store_->contains(key);
}

// List operations
bool DistributedCache::lpush(const std::string& key, const std::string& value) {
    std::shared_ptr<data_structures::ConcurrentLinkedList<std::string>> list;
    
    if (!list_store_->get(key, list)) {
        list = std::make_shared<data_structures::ConcurrentLinkedList<std::string>>();
        list_store_->insert(key, list);
    }
    
    list->push_front(value);
    return true;
}

bool DistributedCache::rpush(const std::string& key, const std::string& value) {
    std::shared_ptr<data_structures::ConcurrentLinkedList<std::string>> list;
    
    if (!list_store_->get(key, list)) {
        list = std::make_shared<data_structures::ConcurrentLinkedList<std::string>>();
        list_store_->insert(key, list);
    }
    
    list->push_back(value);
    return true;
}

std::optional<std::string> DistributedCache::lpop(const std::string& key) {
    std::shared_ptr<data_structures::ConcurrentLinkedList<std::string>> list;
    
    if (!list_store_->get(key, list)) {
        return std::nullopt;
    }
    
    std::string value;
    if (list->pop_front(value)) {
        return value;
    }
    
    return std::nullopt;
}

std::optional<std::string> DistributedCache::rpop(const std::string& key) {
    std::shared_ptr<data_structures::ConcurrentLinkedList<std::string>> list;
    
    if (!list_store_->get(key, list)) {
        return std::nullopt;
    }
    
    std::string value;
    // For now, use pop_front since we don't have pop_back in ConcurrentLinkedList
    // In a real implementation, we'd add pop_back to ConcurrentLinkedList
    if (list->pop_front(value)) {
        return value;
    }
    
    return std::nullopt;
}

size_t DistributedCache::llen(const std::string& key) {
    std::shared_ptr<data_structures::ConcurrentLinkedList<std::string>> list;
    
    if (!list_store_->get(key, list)) {
        return 0;
    }
    
    return list->size();
}

std::vector<std::string> DistributedCache::lrange(const std::string& key, int start, int stop) {
    std::shared_ptr<data_structures::ConcurrentLinkedList<std::string>> list;
    std::vector<std::string> result;
    
    if (!list_store_->get(key, list)) {
        return result;
    }
    
    // For now, return empty vector as we'd need an iterator or at() method
    // In a real implementation, we'd add these to ConcurrentLinkedList
    return result;
}

// Set operations
bool DistributedCache::sadd(const std::string& key, const std::string& member) {
    std::shared_ptr<data_structures::ConcurrentHashMap<std::string, bool>> set;
    
    if (!set_store_->get(key, set)) {
        set = std::make_shared<data_structures::ConcurrentHashMap<std::string, bool>>();
        set_store_->insert(key, set);
    }
    
    set->insert(member, true);
    return true;
}

bool DistributedCache::srem(const std::string& key, const std::string& member) {
    std::shared_ptr<data_structures::ConcurrentHashMap<std::string, bool>> set;
    
    if (!set_store_->get(key, set)) {
        return false;
    }
    
    return set->remove(member);
}

bool DistributedCache::sismember(const std::string& key, const std::string& member) {
    std::shared_ptr<data_structures::ConcurrentHashMap<std::string, bool>> set;
    
    if (!set_store_->get(key, set)) {
        return false;
    }
    
    return set->contains(member);
}

size_t DistributedCache::scard(const std::string& key) {
    std::shared_ptr<data_structures::ConcurrentHashMap<std::string, bool>> set;
    
    if (!set_store_->get(key, set)) {
        return 0;
    }
    
    return set->size();
}

std::vector<std::string> DistributedCache::smembers(const std::string& key) {
    std::shared_ptr<data_structures::ConcurrentHashMap<std::string, bool>> set;
    
    if (!set_store_->get(key, set)) {
        return std::vector<std::string>();
    }
    
    return set->keys();
}

// General operations
std::vector<std::string> DistributedCache::keys() {
    auto string_keys = string_store_->keys();
    auto list_keys = list_store_->keys();
    auto set_keys = set_store_->keys();
    
    std::vector<std::string> all_keys;
    all_keys.insert(all_keys.end(), string_keys.begin(), string_keys.end());
    all_keys.insert(all_keys.end(), list_keys.begin(), list_keys.end());
    all_keys.insert(all_keys.end(), set_keys.begin(), set_keys.end());
    
    return all_keys;
}

bool DistributedCache::flush() {
    string_store_->clear();
    list_store_->clear();
    set_store_->clear();
    return true;
}

size_t DistributedCache::dbsize() {
    return string_store_->size() + list_store_->size() + set_store_->size();
}

// Increment/Decrement operations
std::optional<int64_t> DistributedCache::incr(const std::string& key) {
    return incrby(key, 1);
}

std::optional<int64_t> DistributedCache::decr(const std::string& key) {
    return incrby(key, -1);
}

std::optional<int64_t> DistributedCache::incrby(const std::string& key, int64_t increment) {
    std::shared_ptr<CacheEntry<std::string>> entry;
    int64_t current = 0;
    
    if (string_store_->get(key, entry)) {
        if (entry->is_expired()) {
            string_store_->remove(key);
        } else {
            try {
                current = std::stoll(entry->value);
            } catch (...) {
                return std::nullopt; // Not an integer
            }
        }
    }
    
    int64_t new_value = current + increment;
    set(key, std::to_string(new_value));
    return new_value;
}

// TTL operations
bool DistributedCache::expire(const std::string& key, int seconds) {
    std::shared_ptr<CacheEntry<std::string>> entry;
    
    if (!string_store_->get(key, entry)) {
        return false;
    }
    
    auto expiration = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    entry->expiration = expiration;
    entry->has_expiration = true;
    return true;
}

std::optional<int> DistributedCache::ttl(const std::string& key) {
    std::shared_ptr<CacheEntry<std::string>> entry;
    
    if (!string_store_->get(key, entry)) {
        return std::nullopt;
    }
    
    if (!entry->has_expiration) {
        return -1; // No expiration set
    }
    
    auto now = std::chrono::steady_clock::now();
    if (now > entry->expiration) {
        return -2; // Key expired
    }
    
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        entry->expiration - now).count();
    return static_cast<int>(remaining);
}

bool DistributedCache::persist(const std::string& key) {
    std::shared_ptr<CacheEntry<std::string>> entry;
    
    if (!string_store_->get(key, entry)) {
        return false;
    }
    
    entry->has_expiration = false;
    return true;
}

void DistributedCache::cleanup_expired() {
    auto all_keys = string_store_->keys();
    for (const auto& key : all_keys) {
        std::shared_ptr<CacheEntry<std::string>> entry;
        if (string_store_->get(key, entry) && entry->is_expired()) {
            string_store_->remove(key);
        }
    }
}

bool DistributedCache::is_expired(const std::string& key) {
    std::shared_ptr<CacheEntry<std::string>> entry;
    if (string_store_->get(key, entry)) {
        return entry->is_expired();
    }
    return false;
}








} // namespace services


