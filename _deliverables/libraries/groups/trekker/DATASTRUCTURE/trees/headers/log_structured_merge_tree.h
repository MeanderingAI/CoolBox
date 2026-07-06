#ifndef DATA_STRUCTURES_LOG_STRUCTURED_MERGE_TREE_H
#define DATA_STRUCTURES_LOG_STRUCTURED_MERGE_TREE_H

#include <cstddef>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace data_structures {

template<typename K, typename V>
class LogStructuredMergeTree {
public:
    explicit LogStructuredMergeTree(size_t memtable_threshold = 8)
        : memtable_threshold_(memtable_threshold == 0 ? 1 : memtable_threshold),
          live_entries_(0) {
    }

    void put(const K& key, const V& value);
    bool remove(const K& key);

    bool get(const K& key, V& value) const;
    bool contains(const K& key) const;

    void flush();
    void compact();
    void clear();

    size_t size() const {
        return live_entries_;
    }

    bool empty() const {
        return live_entries_ == 0;
    }

    size_t memtable_size() const {
        return memtable_.size();
    }

    size_t sstable_count() const {
        return sstables_.size();
    }

private:
    struct Entry {
        V value;
        bool tombstone;
    };

    using Table = std::map<K, Entry>;

    size_t memtable_threshold_;
    size_t live_entries_;
    Table memtable_;
    std::vector<Table> sstables_;

    std::optional<Entry> find_entry(const K& key) const;
    void maybe_flush();
};

} // namespace data_structures

#endif // DATA_STRUCTURES_LOG_STRUCTURED_MERGE_TREE_H
