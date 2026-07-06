#include "btree.h"

namespace data_structures {

template<typename Key, typename Value, int Order>
void BPlusTree<Key, Value, Order>::insert(const Key& key, const Value& value) {
	if (root_ == nullptr) {
		root_ = std::make_shared<Node>(true);
		root_->values.push_back({key, value});
		size_++;
		return;
	}

	// Check if key already exists
	if (search(key).has_value()) {
		// Update existing value
		auto leaf = find_leaf_node(root_, key);
		for (auto& pair : leaf->values) {
			if (pair.first == key) {
				pair.second = value;
				return;
			}
		}
	}

	if (static_cast<int>(root_->keys.size()) >= Order) {
		auto new_root = std::make_shared<Node>(false);
		new_root->children.push_back(root_);
		split_child(new_root, 0);
		root_ = new_root;
	}

	insert_non_full(root_, key, value);
	size_++;
}

template<typename Key, typename Value, int Order>
void BPlusTree<Key, Value, Order>::insert_non_full(std::shared_ptr<Node>& node, const Key& key, const Value& value) {
	if (node->is_leaf) {
		auto it = std::lower_bound(node->values.begin(), node->values.end(), 
			std::make_pair(key, value), [](const auto& a, const auto& b) { return a.first < b.first; });
		node->values.insert(it, {key, value});
	} else {
		int idx = find_child_index(node, key);
		if (idx < static_cast<int>(node->children.size()) && 
			static_cast<int>(node->children[idx]->keys.size()) >= Order) {
			split_child(node, idx);
			if (key > node->keys[idx]) {
				idx++;
			}
		}
		insert_non_full(node->children[idx], key, value);
	}
}

template<typename Key, typename Value, int Order>
void BPlusTree<Key, Value, Order>::split_child(std::shared_ptr<Node>& parent, int idx) {
	auto child = parent->children[idx];
	auto new_child = std::make_shared<Node>(child->is_leaf);

	int mid = Order / 2;

	if (child->is_leaf) {
		// Split leaf node
		new_child->values.assign(child->values.begin() + mid, child->values.end());
		child->values.erase(child->values.begin() + mid, child->values.end());

		// Insert separator key in parent
		parent->keys.insert(parent->keys.begin() + idx, new_child->values.front().first);
		parent->children.insert(parent->children.begin() + idx + 1, new_child);

		// Link leaf nodes
		new_child->next_leaf = child->next_leaf;
		new_child->prev_leaf = child;
		if (child->next_leaf) {
			child->next_leaf->prev_leaf = new_child;
		}
		child->next_leaf = new_child;
	} else {
		// Split internal node
		new_child->keys.assign(child->keys.begin() + mid + 1, child->keys.end());
		child->keys.erase(child->keys.begin() + mid, child->keys.end());

		new_child->children.assign(child->children.begin() + mid + 1, child->children.end());
		child->children.erase(child->children.begin() + mid + 1, child->children.end());

		parent->keys.insert(parent->keys.begin() + idx, child->keys.back());
		child->keys.pop_back();
		parent->children.insert(parent->children.begin() + idx + 1, new_child);
	}
}

template<typename Key, typename Value, int Order>
std::optional<Value> BPlusTree<Key, Value, Order>::search(const Key& key) const {
	if (root_ == nullptr) {
		return std::nullopt;
	}
	return search_helper(root_, key);
}

template<typename Key, typename Value, int Order>
std::optional<Value> BPlusTree<Key, Value, Order>::search_helper(const std::shared_ptr<Node>& node, const Key& key) const {
	if (node->is_leaf) {
		for (const auto& pair : node->values) {
			if (pair.first == key) {
				return pair.second;
			}
		}
		return std::nullopt;
	}

	int idx = find_child_index(node, key);
	if (idx < static_cast<int>(node->children.size())) {
		return search_helper(node->children[idx], key);
	}
	return std::nullopt;
}

template<typename Key, typename Value, int Order>
int BPlusTree<Key, Value, Order>::find_child_index(const std::shared_ptr<Node>& node, const Key& key) const {
	int idx = 0;
	while (idx < static_cast<int>(node->keys.size()) && key > node->keys[idx]) {
		idx++;
	}
	return idx;
}

template<typename Key, typename Value, int Order>
std::vector<std::pair<Key, Value>> BPlusTree<Key, Value, Order>::range_query(const Key& min_key, const Key& max_key) const {
	std::vector<std::pair<Key, Value>> result;
	
	if (root_ == nullptr) {
		return result;
	}

	auto leaf = find_leaf_node(root_, min_key);
	
	// Traverse leaves and collect values in range
	while (leaf) {
		for (const auto& pair : leaf->values) {
			if (pair.first >= min_key && pair.first <= max_key) {
				result.push_back(pair);
			} else if (pair.first > max_key) {
				return result;
			}
		}
		leaf = leaf->next_leaf;
	}

	return result;
}

template<typename Key, typename Value, int Order>
std::vector<std::pair<Key, Value>> BPlusTree<Key, Value, Order>::all_entries() const {
	std::vector<std::pair<Key, Value>> result;
	
	if (root_ == nullptr) {
		return result;
	}

	// Find leftmost leaf
	auto leaf = root_;
	while (!leaf->is_leaf) {
		leaf = leaf->children.front();
	}

	// Traverse all leaves
	while (leaf) {
		result.insert(result.end(), leaf->values.begin(), leaf->values.end());
		leaf = leaf->next_leaf;
	}

	return result;
}

template<typename Key, typename Value, int Order>
std::shared_ptr<typename BPlusTree<Key, Value, Order>::Node> BPlusTree<Key, Value, Order>::find_leaf_node(
	const std::shared_ptr<Node>& node, const Key& key) const {
	if (node->is_leaf) {
		return node;
	}

	int idx = find_child_index(node, key);
	if (idx < static_cast<int>(node->children.size())) {
		return find_leaf_node(node->children[idx], key);
	}
	
	// Return the rightmost leaf if key is beyond all keys
	auto current = node->children.back();
	while (!current->is_leaf) {
		current = current->children.back();
	}
	return current;
}

template<typename Key, typename Value, int Order>
bool BPlusTree<Key, Value, Order>::remove(const Key& key) {
	if (root_ == nullptr) {
		return false;
	}

	bool removed = remove_helper(root_, key);
	
	// If root is empty and has a child, make the child the new root
	if (root_->keys.empty() && !root_->children.empty()) {
		root_ = root_->children.front();
	}
	
	// If root is empty and is a leaf, it's now empty
	if (root_->keys.empty() && root_->is_leaf && root_->values.empty()) {
		root_ = nullptr;
	}

	if (removed) {
		size_--;
	}

	return removed;
}

template<typename Key, typename Value, int Order>
bool BPlusTree<Key, Value, Order>::remove_helper(std::shared_ptr<Node>& node, const Key& key) {
	if (node->is_leaf) {
		auto it = std::find_if(node->values.begin(), node->values.end(),
			[&key](const auto& pair) { return pair.first == key; });
		
		if (it != node->values.end()) {
			node->values.erase(it);
			return true;
		}
		return false;
	}

	int idx = find_child_index(node, key);
	
	if (idx < static_cast<int>(node->keys.size()) && node->keys[idx] == key) {
		return remove_from_non_leaf(node, idx);
	}

	if (idx < static_cast<int>(node->children.size())) {
		return remove_helper(node->children[idx], key);
	}

	return false;
}

template<typename Key, typename Value, int Order>
bool BPlusTree<Key, Value, Order>::remove_from_leaf(std::shared_ptr<Node>& node, int idx) {
	auto it = std::find_if(node->values.begin(), node->values.end(),
		[&](const auto& pair) { return pair.first == node->values[idx].first; });
	
	if (it != node->values.end()) {
		node->values.erase(it);
		return true;
	}
	return false;
}

template<typename Key, typename Value, int Order>
bool BPlusTree<Key, Value, Order>::remove_from_non_leaf(std::shared_ptr<Node>& node, int idx) {
	Key key = node->keys[idx];
	
	if (static_cast<int>(node->children[idx]->keys.size()) >= Order) {
		Key predecessor = get_predecessor(node, idx);
		node->keys[idx] = predecessor;
		return remove_helper(node->children[idx], predecessor);
	} else if (static_cast<int>(node->children[idx + 1]->keys.size()) >= Order) {
		Key successor = get_successor(node, idx);
		node->keys[idx] = successor;
		return remove_helper(node->children[idx + 1], successor);
	} else {
		merge(node, idx);
		return remove_helper(node->children[idx], key);
	}
}

template<typename Key, typename Value, int Order>
Key BPlusTree<Key, Value, Order>::get_predecessor(std::shared_ptr<Node>& node, int idx) const {
	auto current = node->children[idx];
	while (!current->is_leaf) {
		current = current->children.back();
	}
	return current->values.back().first;
}

template<typename Key, typename Value, int Order>
Key BPlusTree<Key, Value, Order>::get_successor(std::shared_ptr<Node>& node, int idx) const {
	auto current = node->children[idx + 1];
	while (!current->is_leaf) {
		current = current->children.front();
	}
	return current->values.front().first;
}

template<typename Key, typename Value, int Order>
void BPlusTree<Key, Value, Order>::merge(std::shared_ptr<Node>& node, int idx) {
	auto child = node->children[idx];
	auto sibling = node->children[idx + 1];

	// Move the key from this node to the child
	child->keys.push_back(node->keys[idx]);

	// Copy keys and children from sibling to child
	child->keys.insert(child->keys.end(), sibling->keys.begin(), sibling->keys.end());
	
	if (!child->is_leaf) {
		child->children.insert(child->children.end(), sibling->children.begin(), sibling->children.end());
	} else {
		// For leaf nodes, merge values
		child->values.insert(child->values.end(), sibling->values.begin(), sibling->values.end());
		child->next_leaf = sibling->next_leaf;
		if (sibling->next_leaf) {
			sibling->next_leaf->prev_leaf = child;
		}
	}

	// Remove the key from this node
	node->keys.erase(node->keys.begin() + idx);
	node->children.erase(node->children.begin() + idx + 1);
}

template<typename Key, typename Value, int Order>
void BPlusTree<Key, Value, Order>::clear() {
	root_ = nullptr;
	size_ = 0;
}

// Explicit template instantiations for common types
template class BPlusTree<std::string, std::string, 4>;
template class BPlusTree<std::string, std::vector<std::string>, 4>;
template class BPlusTree<int, std::string, 4>;
template class BPlusTree<int, int, 4>;

} // namespace data_structures
