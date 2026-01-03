#include "data_structures/concurrent_hash_map.h"

#include "data_structures/concurrent_linked_list.h"
#include "services/cache_server/distributed_cache.h"
#include "services/dns/dns_server.h"
#include "../../_binaries/services/proxy_service/include/proxy_server.h"

namespace data_structures {

// Template constructor implementation
template<typename K, typename V>
ConcurrentHashMap<K, V>::ConcurrentHashMap(size_t initial_capacity, float load_factor)
	: capacity_(initial_capacity), size_(0), load_factor_(load_factor), buckets_(), hasher_(), size_mutex_() {
	buckets_.reserve(capacity_);
	for (size_t i = 0; i < capacity_; ++i) {
		buckets_.emplace_back(std::make_unique<Bucket>());
	}
}

template<typename K, typename V>
void ConcurrentHashMap<K, V>::insert(const K& key, const V& value) {
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








// Explicit template instantiations must be inside the data_structures namespace

// Explicit template instantiations must be inside the data_structures namespace






// ...existing code...

// Explicit template instantiations for supported types (must be inside the namespace)



// Explicit template instantiations for all required types
template class ConcurrentHashMap<int, int>;
template class ConcurrentHashMap<int, double>;
template class ConcurrentHashMap<int, std::string>;
template class ConcurrentHashMap<std::string, int>;
template class ConcurrentHashMap<std::string, double>;
template class ConcurrentHashMap<std::string, std::string>;
template class ConcurrentHashMap<std::string, bool>;
template class ConcurrentHashMap<std::string, std::shared_ptr<ConcurrentHashMap<std::string, bool>>>;
template class ConcurrentHashMap<std::string, std::shared_ptr<ConcurrentLinkedList<std::string>>>;
template class ConcurrentHashMap<std::string, std::shared_ptr<services::CacheEntry<std::string>>>;
template class ConcurrentHashMap<std::string, std::vector<services::dns::DNSRecord>>;
template class ConcurrentHashMap<std::string, std::shared_ptr<services::proxy::CachedResponse>>;
} // namespace data_structures




