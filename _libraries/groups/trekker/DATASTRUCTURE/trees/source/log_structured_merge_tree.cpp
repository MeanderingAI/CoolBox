#include "../headers/log_structured_merge_tree.h"

#include <string>
#include <utility>

namespace data_structures {

template<typename K, typename V>
void LogStructuredMergeTree<K, V>::put(const K& key, const V& value) {
    const bool existed = contains(key);
    memtable_[key] = Entry{value, false};
    if (!existed) {
        ++live_entries_;
    }
    maybe_flush();
}

template<typename K, typename V>
bool LogStructuredMergeTree<K, V>::remove(const K& key) {
    const bool existed = contains(key);
    if (!existed) {
        return false;
    }

    memtable_[key] = Entry{V{}, true};
    --live_entries_;
    maybe_flush();
    return true;
}

template<typename K, typename V>
bool LogStructuredMergeTree<K, V>::get(const K& key, V& value) const {
    const auto entry = find_entry(key);
    if (!entry.has_value() || entry->tombstone) {
        return false;
    }

    value = entry->value;
    return true;
}

template<typename K, typename V>
bool LogStructuredMergeTree<K, V>::contains(const K& key) const {
    const auto entry = find_entry(key);
    return entry.has_value() && !entry->tombstone;
}

template<typename K, typename V>
void LogStructuredMergeTree<K, V>::flush() {
    if (memtable_.empty()) {
        return;
    }

    sstables_.insert(sstables_.begin(), std::move(memtable_));
    memtable_.clear();
}

template<typename K, typename V>
void LogStructuredMergeTree<K, V>::compact() {
    Table merged;

    for (auto it = sstables_.rbegin(); it != sstables_.rend(); ++it) {
        for (const auto& [key, entry] : *it) {
            merged[key] = entry;
        }
    }

    for (const auto& [key, entry] : memtable_) {
        merged[key] = entry;
    }

    Table compacted;
    for (const auto& [key, entry] : merged) {
        if (!entry.tombstone) {
            compacted[key] = entry;
        }
    }

    sstables_.clear();
    if (!compacted.empty()) {
        sstables_.push_back(std::move(compacted));
    }
}

template<typename K, typename V>
void LogStructuredMergeTree<K, V>::clear() {
    memtable_.clear();
    sstables_.clear();
    live_entries_ = 0;
}

template<typename K, typename V>
std::optional<typename LogStructuredMergeTree<K, V>::Entry> LogStructuredMergeTree<K, V>::find_entry(const K& key) const {
    auto memtable_it = memtable_.find(key);
    if (memtable_it != memtable_.end()) {
        return memtable_it->second;
    }

    for (const auto& table : sstables_) {
        auto it = table.find(key);
        if (it != table.end()) {
            return it->second;
        }
    }

    return std::nullopt;
}

template<typename K, typename V>
void LogStructuredMergeTree<K, V>::maybe_flush() {
    if (memtable_.size() >= memtable_threshold_) {
        flush();
    }
}

template class LogStructuredMergeTree<int, int>;
template class LogStructuredMergeTree<std::string, int>;
template class LogStructuredMergeTree<std::string, std::string>;

} // namespace data_structures
