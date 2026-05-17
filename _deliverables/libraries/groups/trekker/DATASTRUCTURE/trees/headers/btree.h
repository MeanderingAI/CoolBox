#ifndef DATA_STRUCTURES_BTREE_H
#define DATA_STRUCTURES_BTREE_H

#include <vector>
#include <memory>
#include <optional>
#include <algorithm>
#include <stdexcept>

namespace data_structures {

template<typename Key, typename Value, int Order = 4>
class BPlusTree {
private:
	struct Node {
		std::vector<Key> keys;
		std::vector<std::shared_ptr<Node>> children;
		std::vector<std::pair<Key, Value>> values; // For leaf nodes only
		std::shared_ptr<Node> next_leaf; // For leaf nodes only
		std::shared_ptr<Node> prev_leaf; // For leaf nodes only
		bool is_leaf;

		explicit Node(bool leaf = true) 
			: is_leaf(leaf), next_leaf(nullptr), prev_leaf(nullptr) {}
	};

public:
	BPlusTree() : root_(nullptr), size_(0) {}

	// Insert a key-value pair
	void insert(const Key& key, const Value& value);

	// Search for a value by key
	std::optional<Value> search(const Key& key) const;

	// Delete a key-value pair
	bool remove(const Key& key);

	// Range query: return all values with keys in [min_key, max_key]
	std::vector<std::pair<Key, Value>> range_query(const Key& min_key, const Key& max_key) const;

	// Get all key-value pairs in sorted order
	std::vector<std::pair<Key, Value>> all_entries() const;

	// Clear the tree
	void clear();

	// Get number of entries
	size_t size() const { return size_; }

	// Check if tree is empty
	bool empty() const { return size_ == 0; }

private:
	std::shared_ptr<Node> root_;
	size_t size_;

	// Helper functions
	std::optional<Value> search_helper(const std::shared_ptr<Node>& node, const Key& key) const;
	
	int find_child_index(const std::shared_ptr<Node>& node, const Key& key) const;
	
	void insert_non_full(std::shared_ptr<Node>& node, const Key& key, const Value& value);
	
	void split_child(std::shared_ptr<Node>& parent, int idx);
	
	bool remove_helper(std::shared_ptr<Node>& node, const Key& key);
	
	bool remove_from_leaf(std::shared_ptr<Node>& node, int idx);
	
	bool remove_from_non_leaf(std::shared_ptr<Node>& node, int idx);
	
	Key get_predecessor(std::shared_ptr<Node>& node, int idx) const;
	
	Key get_successor(std::shared_ptr<Node>& node, int idx) const;
	
	void merge(std::shared_ptr<Node>& node, int idx);
	
	void borrow_from_prev(std::shared_ptr<Node>& node, int child_idx);
	
	void borrow_from_next(std::shared_ptr<Node>& node, int child_idx);
	
	void fill(std::shared_ptr<Node>& node, int idx);
	
	std::shared_ptr<Node> find_leaf_node(const std::shared_ptr<Node>& node, const Key& key) const;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_BTREE_H
