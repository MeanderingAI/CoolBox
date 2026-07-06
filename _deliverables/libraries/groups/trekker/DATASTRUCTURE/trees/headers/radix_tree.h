
#pragma once

#include <memory>
#include <map>
#include <string>
#include <optional>

// A simple radix tree (compact prefix tree) for string keys
template <typename Value>
class radix_tree {
    struct Node {
        std::map<std::string, std::shared_ptr<Node>> children;
        std::optional<Value> value;
    };
    std::shared_ptr<Node> root_;
public:
    radix_tree() : root_(std::make_shared<Node>()) {}

    // Insert a key-value pair
    void insert(const std::string& key, const Value& value) {
        auto node = root_;
        size_t pos = 0;
        while (pos < key.size()) {
            bool found = false;
            for (auto& [edge, child] : node->children) {
                size_t match = common_prefix(key, pos, edge);
                if (match > 0) {
                    if (match < edge.size()) {
                        // Split edge
                        auto split = std::make_shared<Node>(*child);
                        child->children[edge.substr(match)] = split;
                        child->children.erase(edge);
                        child = std::make_shared<Node>();
                        child->children[edge.substr(match)] = split;
                    }
                    pos += match;
                    node = child;
                    found = true;
                    break;
                }
            }
            if (!found) {
                node->children[key.substr(pos)] = std::make_shared<Node>();
                node = node->children[key.substr(pos)];
                break;
            }
        }
        node->value = value;
    }

    // Find a value by key
    std::optional<Value> find(const std::string& key) const {
        auto node = root_;
        size_t pos = 0;
        while (pos < key.size()) {
            bool found = false;
            for (const auto& [edge, child] : node->children) {
                size_t match = common_prefix(key, pos, edge);
                if (match == edge.size() && key.substr(pos, match) == edge) {
                    pos += match;
                    node = child;
                    found = true;
                    break;
                }
            }
            if (!found) return std::nullopt;
        }
        return node->value;
    }

    // Remove a key (returns true if removed)
    bool remove(const std::string& key) {
        return remove_internal(root_, key, 0);
    }

private:
    // Helper for remove
    bool remove_internal(std::shared_ptr<Node> node, const std::string& key, size_t pos) {
        if (pos == key.size()) {
            if (node->value) {
                node->value.reset();
                return true;
            }
            return false;
        }
        for (auto& [edge, child] : node->children) {
            size_t match = common_prefix(key, pos, edge);
            if (match == edge.size() && key.substr(pos, match) == edge) {
                return remove_internal(child, key, pos + match);
            }
        }
        return false;
    }

    // Returns length of common prefix between key[pos:] and edge
    static size_t common_prefix(const std::string& key, size_t pos, const std::string& edge) {
        size_t i = 0;
        while (i < edge.size() && pos + i < key.size() && key[pos + i] == edge[i]) ++i;
        return i;
    }
};
