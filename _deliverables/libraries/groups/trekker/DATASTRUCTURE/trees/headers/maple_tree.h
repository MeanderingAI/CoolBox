// maple_tree.h
// Maple Tree implementation for the DATASTRUCTURE package
// Author: [Your Name]
// License: Same as project
#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

// A simple Maple Tree implementation (Maple Tree is a B-tree variant used in Linux kernel)
template <typename Key, typename Value, size_t Order = 32>
class MapleTree {
public:
    struct Node {
        bool is_leaf;
        std::vector<Key> keys;
        std::vector<std::shared_ptr<Node>> children;
        std::vector<Value> values; // Only for leaves
        Node(bool leaf) : is_leaf(leaf) {}
    };

    MapleTree() : root(std::make_shared<Node>(true)) {}


    std::optional<Value> find(const Key& key) const {
        return find_internal(root, key);
    }

    // operator[] for test compatibility (throws if not found)
    Value operator[](const Key& key) const {
        auto result = find(key);
        if (!result) throw std::out_of_range("Key not found in MapleTree");
        return *result;
    }

    // Remove method for test compatibility (returns true if removed)
    bool remove(const Key& key) {
        if (!find(key)) {
            return false;
        }
        std::vector<std::pair<Key, Value>> entries;
        collect_entries(root, entries);
        root = std::make_shared<Node>(true);
        for (const auto& entry : entries) {
            if (entry.first != key) {
                insert(entry.first, entry.second);
            }
        }
        return true;
    }

    void insert(const Key& key, const Value& value) {
        if (root->keys.size() == 2 * Order - 1) {
            auto s = std::make_shared<Node>(false);
            s->children.push_back(root);
            split_child(s, 0);
            root = s;
        }
        insert_nonfull(root, key, value);
    }

private:
    void collect_entries(const std::shared_ptr<Node>& node,
                         std::vector<std::pair<Key, Value>>& entries) const {
        if (node->is_leaf) {
            for (size_t i = 0; i < node->keys.size(); ++i) {
                entries.emplace_back(node->keys[i], node->values[i]);
            }
            return;
        }
        for (const auto& child : node->children) {
            collect_entries(child, entries);
        }
    }
    std::shared_ptr<Node> root;

    std::optional<Value> find_internal(std::shared_ptr<Node> node, const Key& key) const {
        size_t i = 0;
        while (i < node->keys.size() && key > node->keys[i]) ++i;
        if (i < node->keys.size() && key == node->keys[i]) {
            if (node->is_leaf) return node->values[i];
            else return find_internal(node->children[i + 1], key);
        }
        if (node->is_leaf) return std::nullopt;
        return find_internal(node->children[i], key);
    }

    void insert_nonfull(std::shared_ptr<Node> node, const Key& key, const Value& value) {
        if (node->is_leaf) {
            auto position = std::lower_bound(node->keys.begin(), node->keys.end(), key);
            const size_t index = static_cast<size_t>(position - node->keys.begin());
            if (position != node->keys.end() && *position == key) {
                node->values[index] = value;
                return;
            }
            node->keys.insert(position, key);
            node->values.insert(node->values.begin() + index, value);
            return;
        }

        size_t index = 0;
        while (index < node->keys.size() && key >= node->keys[index]) ++index;
        if (node->children[index]->keys.size() == 2 * Order - 1) {
            split_child(node, index);
            if (key >= node->keys[index]) ++index;
        }
        insert_nonfull(node->children[index], key, value);
    }

    void split_child(std::shared_ptr<Node> parent, size_t i) {
        auto y = parent->children[i];
        auto z = std::make_shared<Node>(y->is_leaf);
        if (y->is_leaf) {
            z->keys.assign(y->keys.begin() + Order, y->keys.end());
            z->values.assign(y->values.begin() + Order, y->values.end());
            const Key separator = z->keys.front();
            y->keys.resize(Order);
            y->values.resize(Order);
            parent->keys.insert(parent->keys.begin() + i, separator);
        } else {
            const Key separator = y->keys[Order - 1];
            z->keys.assign(y->keys.begin() + Order, y->keys.end());
            y->keys.resize(Order - 1);
            z->children.assign(y->children.begin() + Order, y->children.end());
            y->children.resize(Order);
            parent->keys.insert(parent->keys.begin() + i, separator);
        }
        parent->children.insert(parent->children.begin() + i + 1, z);
    }
};
