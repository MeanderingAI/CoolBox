// maple_tree.h
// Maple Tree implementation for the DATASTRUCTURE package
// Author: [Your Name]
#pragma once

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
        return remove_internal(root, key);
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
    // Remove helper (simple linear search for demonstration)
    bool remove_internal(std::shared_ptr<Node> node, const Key& key) {
        size_t i = 0;
        while (i < node->keys.size() && key != node->keys[i]) ++i;
        if (i < node->keys.size()) {
            if (node->is_leaf) {
                node->keys.erase(node->keys.begin() + i);
                node->values.erase(node->values.begin() + i);
                return true;
            } else {
                // Not implemented: remove from internal node
                return false;
            }
        }
        if (node->is_leaf) return false;
        for (auto& child : node->children) {
            if (remove_internal(child, key)) return true;
        }
        return false;
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
        size_t i = node->keys.size();
        if (node->is_leaf) {
            node->keys.insert(node->keys.begin() + i, key);
            node->values.insert(node->values.begin() + i, value);
        } else {
            while (i > 0 && key < node->keys[i - 1]) --i;
            if (node->children[i]->keys.size() == 2 * Order - 1) {
                split_child(node, i);
                if (key > node->keys[i]) ++i;
            }
            insert_nonfull(node->children[i], key, value);
        }
    }

    void split_child(std::shared_ptr<Node> parent, size_t i) {
        auto y = parent->children[i];
        auto z = std::make_shared<Node>(y->is_leaf);
        z->keys.assign(y->keys.begin() + Order, y->keys.end());
        y->keys.resize(Order - 1);
        if (y->is_leaf) {
            z->values.assign(y->values.begin() + Order, y->values.end());
            y->values.resize(Order - 1);
        } else {
            z->children.assign(y->children.begin() + Order, y->children.end());
            y->children.resize(Order);
        }
        parent->children.insert(parent->children.begin() + i + 1, z);
        parent->keys.insert(parent->keys.begin() + i, y->keys[Order - 1]);
    }
};
