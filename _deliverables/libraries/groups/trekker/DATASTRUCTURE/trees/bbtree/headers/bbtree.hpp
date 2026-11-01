#ifndef DATA_STRUCTURES_BBTREE_HPP
#define DATA_STRUCTURES_BBTREE_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace data_structures {

// Single-threaded B+ ordered map; unrelated to trekker::gis::BBTree.
template <typename Key, typename T, typename Compare = std::less<Key>,
          std::size_t Degree = 8>
class BBTreeMap {
    static_assert(Degree >= 2, "BBTreeMap requires a minimum degree of two");
    static_assert(Degree <= 128, "BBTreeMap degree must not exceed 128");

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<const Key, T>;
    using size_type = std::size_t;

private:
    using Entry = std::shared_ptr<value_type>;
    static constexpr size_type max_entries = 2 * Degree - 1;
    static constexpr bool natural_order =
        std::is_same_v<Compare, std::less<Key>> || std::is_same_v<Compare, std::less<void>>;
    static constexpr bool string_hints = natural_order && std::is_same_v<Key, std::string>;
    static constexpr bool integer_hints = natural_order && std::is_integral_v<Key> &&
        std::is_unsigned_v<Key> && sizeof(Key) <= sizeof(std::uint64_t);

    struct Node {
        bool leaf = true;
        std::vector<Entry> entries;
        std::vector<std::unique_ptr<Node>> children;
        std::array<std::uint64_t, max_entries + 1> hints{};
        size_type prefix = 0;
        Node* next = nullptr;

        Node() {
            entries.reserve(max_entries + 1);
            children.reserve(max_entries + 2);
        }
    };

    struct Step {
        Node* node;
        size_type child;
    };

public:
    template <bool Const>
    class Iterator {
        friend class BBTreeMap;
        template <bool> friend class Iterator;
        Node* node_ = nullptr;
        size_type index_ = 0;

        Iterator(Node* node, size_type index) : node_(node), index_(index) {
            if (node_ && index_ == node_->entries.size()) {
                node_ = node_->next;
                index_ = 0;
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename BBTreeMap::value_type;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<Const, const value_type&, value_type&>;
        using pointer = std::conditional_t<Const, const value_type*, value_type*>;

        Iterator() = default;
        template <bool C = Const, std::enable_if_t<C, int> = 0>
        Iterator(const Iterator<false>& other) : node_(other.node_), index_(other.index_) {}

        reference operator*() const { return *node_->entries[index_]; }
        pointer operator->() const { return node_->entries[index_].get(); }
        Iterator& operator++() {
            ++index_;
            if (index_ == node_->entries.size()) {
                node_ = node_->next;
                index_ = 0;
            }
            return *this;
        }
        Iterator operator++(int) {
            auto previous = *this;
            ++*this;
            return previous;
        }
        template <bool C>
        bool operator==(const Iterator<C>& other) const {
            return node_ == other.node_ && index_ == other.index_;
        }
        template <bool C>
        bool operator!=(const Iterator<C>& other) const { return !(*this == other); }
    };

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    BBTreeMap() = default;
    explicit BBTreeMap(Compare compare) : compare_(std::move(compare)) {}
    BBTreeMap(const BBTreeMap& other) : compare_(other.compare_) {
        for (const auto& entry : other) {
            insert(entry.first, entry.second);
        }
    }
    BBTreeMap(BBTreeMap&& other) noexcept(std::is_nothrow_move_constructible_v<Compare>)
        : compare_(std::move(other.compare_)), root_(std::move(other.root_)),
          size_(std::exchange(other.size_, 0)) {}

    BBTreeMap& operator=(BBTreeMap other) noexcept(std::is_nothrow_swappable_v<Compare>) {
        swap(other);
        return *this;
    }
    void swap(BBTreeMap& other) noexcept(std::is_nothrow_swappable_v<Compare>) {
        using std::swap;
        swap(compare_, other.compare_);
        root_.swap(other.root_);
        swap(size_, other.size_);
    }

    size_type size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    void clear() noexcept {
        root_.reset();
        size_ = 0;
    }
    size_type height() const noexcept {
        size_type result = 0;
        for (auto* node = root_.get(); node; node = node->leaf ? nullptr : node->children.front().get()) {
            ++result;
        }
        return result;
    }

    iterator begin() noexcept { return iterator(first_leaf(), 0); }
    const_iterator begin() const noexcept { return const_iterator(first_leaf(), 0); }
    const_iterator cbegin() const noexcept { return begin(); }
    iterator end() noexcept { return {}; }
    const_iterator end() const noexcept { return {}; }
    const_iterator cend() const noexcept { return end(); }

    iterator lower_bound(const Key& key) { return bound<false>(key, false); }
    const_iterator lower_bound(const Key& key) const { return bound<true>(key, false); }
    iterator upper_bound(const Key& key) { return bound<false>(key, true); }
    const_iterator upper_bound(const Key& key) const { return bound<true>(key, true); }
    iterator find(const Key& key) {
        auto result = lower_bound(key);
        return result != end() && equal(result->first, key) ? result : end();
    }
    const_iterator find(const Key& key) const {
        auto result = lower_bound(key);
        return result != end() && equal(result->first, key) ? result : end();
    }
    bool contains(const Key& key) const { return find(key) != end(); }
    T& at(const Key& key) {
        const auto result = find(key);
        if (result == end()) {
            throw std::out_of_range("BBTreeMap key not found");
        }
        return result->second;
    }
    const T& at(const Key& key) const {
        const auto result = find(key);
        if (result == end()) {
            throw std::out_of_range("BBTreeMap key not found");
        }
        return result->second;
    }

    template <typename V>
    std::pair<iterator, bool> insert(const Key& key, V&& value) {
        return upsert(key, std::forward<V>(value), false);
    }
    template <typename V>
    std::pair<iterator, bool> insert_or_assign(const Key& key, V&& value) {
        return upsert(key, std::forward<V>(value), true);
    }

    bool erase(const Key& key) {
        std::vector<Step> path;
        Node* leaf = descend(key, &path);
        if (!leaf) {
            return false;
        }
        const size_type index = position(*leaf, key, false);
        if (index == leaf->entries.size() || !equal(leaf->entries[index]->first, key)) {
            return false;
        }
        leaf->entries.erase(leaf->entries.begin() + index);
        --size_;
        refresh(*leaf);
        for (auto step = path.rbegin(); step != path.rend(); ++step) {
            rebalance(*step->node, step->child);
            refresh(*step->node);
        }
        if (root_->leaf && root_->entries.empty()) {
            root_.reset();
        } else if (!root_->leaf && root_->children.size() == 1) {
            auto child = std::move(root_->children.front());
            root_ = std::move(child);
        }
        return true;
    }

    // Checks occupancy, separator identity, ordering, equal leaf depth and links.
    bool validate() const {
        if (!root_) {
            return size_ == 0;
        }
        size_type count = 0;
        size_type leaf_depth = 0;
        const Node* previous = nullptr;
        const auto check = [&](const auto& self, const Node& node, size_type depth,
                               bool root) -> bool {
            if (node.entries.size() > max_entries ||
                (!root && node.entries.size() < Degree - 1)) {
                return false;
            }
            for (size_type i = 1; i < node.entries.size(); ++i) {
                if (!compare_(node.entries[i - 1]->first, node.entries[i]->first)) {
                    return false;
                }
            }
            if (node.leaf) {
                if (node.entries.empty() || !node.children.empty() ||
                    (leaf_depth && leaf_depth != depth)) {
                    return false;
                }
                if (previous && (previous->next != &node ||
                    !compare_(previous->entries.back()->first, node.entries.front()->first))) {
                    return false;
                }
                previous = &node;
                leaf_depth = depth;
                count += node.entries.size();
            } else {
                if (node.children.size() != node.entries.size() + 1 ||
                    (root && node.children.size() < 2)) {
                    return false;
                }
                for (size_type i = 0; i < node.children.size(); ++i) {
                    if (!node.children[i] || (i && node.entries[i - 1] != minimum(*node.children[i])) ||
                        !self(self, *node.children[i], depth + 1, false)) {
                        return false;
                    }
                }
            }
            return true;
        };
        return check(check, *root_, 1, true) && count == size_ && previous && !previous->next;
    }

private:
    Compare compare_{};
    std::unique_ptr<Node> root_;
    size_type size_ = 0;

    bool equal(const Key& a, const Key& b) const {
        return !compare_(a, b) && !compare_(b, a);
    }
    static const Entry& minimum(const Node& node) {
        const Node* leaf = &node;
        while (!leaf->leaf) {
            leaf = leaf->children.front().get();
        }
        return leaf->entries.front();
    }
    Node* first_leaf() const noexcept {
        Node* node = root_.get();
        while (node && !node->leaf) {
            node = node->children.front().get();
        }
        return node;
    }
    static std::uint64_t hint(const Key& key, size_type prefix) noexcept {
        if constexpr (string_hints) {
            std::uint64_t value = 0;
            for (size_type i = 0; i < 8; ++i) {
                value <<= 8;
                if (i < key.size() - prefix) {
                    value |= static_cast<unsigned char>(key[prefix + i]);
                }
            }
            return value;
        } else if constexpr (integer_hints) {
            return static_cast<std::uint64_t>(key);
        } else {
            return 0;
        }
    }
    static void refresh(Node& node) noexcept {
        if (!node.leaf) {
            node.entries.clear();
            for (size_type i = 1; i < node.children.size(); ++i) {
                node.entries.push_back(minimum(*node.children[i]));
            }
        }
        node.prefix = 0;
        if constexpr (string_hints) {
            if (!node.entries.empty()) {
                const auto& first = node.entries.front()->first;
                const auto& last = node.entries.back()->first;
                const size_type length = std::min(first.size(), last.size());
                while (node.prefix < length && first[node.prefix] == last[node.prefix]) {
                    ++node.prefix;
                }
            }
        }
        if constexpr (string_hints || integer_hints) {
            for (size_type i = 0; i < node.entries.size(); ++i) {
                node.hints[i] = hint(node.entries[i]->first, node.prefix);
            }
        }
    }
    size_type position(const Node& node, const Key& key, bool upper) const {
        if constexpr (string_hints) {
            if (!node.entries.empty() && node.prefix) {
                const int prefix_order = key.compare(0, node.prefix, node.entries.front()->first, 0, node.prefix);
                if (prefix_order != 0) {
                    return prefix_order < 0 ? 0 : node.entries.size();
                }
            }
        }
        if constexpr (string_hints || integer_hints) {
            const auto search_hint = hint(key, node.prefix);
            for (size_type i = 0; i < node.entries.size(); ++i) {
                if (node.hints[i] > search_hint ||
                    (node.hints[i] == search_hint &&
                     (upper ? compare_(key, node.entries[i]->first)
                            : !compare_(node.entries[i]->first, key)))) {
                    return i;
                }
            }
            return node.entries.size();
        } else {
            const auto found = upper
                ? std::upper_bound(node.entries.begin(), node.entries.end(), key,
                    [&](const Key& k, const Entry& entry) { return compare_(k, entry->first); })
                : std::lower_bound(node.entries.begin(), node.entries.end(), key,
                    [&](const Entry& entry, const Key& k) { return compare_(entry->first, k); });
            return static_cast<size_type>(found - node.entries.begin());
        }
    }
    Node* descend(const Key& key, std::vector<Step>* path = nullptr) const {
        Node* node = root_.get();
        while (node && !node->leaf) {
            const size_type child = position(*node, key, true);
            if (path) {
                path->push_back({node, child});
            }
            node = node->children[child].get();
        }
        return node;
    }
    template <bool Const>
    Iterator<Const> bound(const Key& key, bool upper) const {
        Node* leaf = descend(key);
        return leaf ? Iterator<Const>(leaf, position(*leaf, key, upper)) : Iterator<Const>{};
    }

    template <typename V>
    std::pair<iterator, bool> upsert(const Key& key, V&& value, bool assign) {
        std::vector<Step> path;
        Node* leaf = descend(key, &path);
        size_type index = leaf ? position(*leaf, key, false) : 0;
        const bool exists = leaf && index < leaf->entries.size() &&
                            equal(leaf->entries[index]->first, key);
        if (exists && !assign) {
            return {iterator(leaf, index), false};
        }
        auto entry = std::make_shared<value_type>(
            exists ? leaf->entries[index]->first : key, std::forward<V>(value));
        if (exists) {
            leaf->entries[index] = std::move(entry);
            refresh(*leaf);
            for (auto step = path.rbegin(); step != path.rend(); ++step) {
                refresh(*step->node);
            }
            return {iterator(leaf, index), false};
        }

        // Allocate possible split nodes before modifying the tree. Reserved node
        // storage and shared separators make structural repair non-throwing.
        size_type splits = 0;
        if (leaf && leaf->entries.size() == max_entries) {
            ++splits;
            for (auto step = path.rbegin(); step != path.rend() &&
                 step->node->entries.size() == max_entries; ++step) {
                ++splits;
            }
        }
        std::vector<std::unique_ptr<Node>> spare;
        const size_type needed = !leaf ? 1 : splits + (splits == path.size() + 1 ? 1 : 0);
        spare.reserve(needed);
        for (size_type i = 0; i < needed; ++i) {
            spare.push_back(std::make_unique<Node>());
        }
        const auto take = [&]() {
            auto node = std::move(spare.back());
            spare.pop_back();
            return node;
        };
        if (!leaf) {
            root_ = take();
            leaf = root_.get();
        }
        leaf->entries.insert(leaf->entries.begin() + index, entry);
        ++size_;
        refresh(*leaf);
        std::unique_ptr<Node> sibling;
        if (leaf->entries.size() > max_entries) {
            sibling = take();
            split(*leaf, *sibling);
        }
        Node* result_leaf = leaf;
        if (sibling && index >= leaf->entries.size()) {
            index -= leaf->entries.size();
            result_leaf = sibling.get();
        }
        for (auto step = path.rbegin(); step != path.rend(); ++step) {
            Node* current = step->node;
            if (sibling) {
                current->children.insert(current->children.begin() + step->child + 1,
                                         std::move(sibling));
            }
            refresh(*current);
            if (current->entries.size() > max_entries) {
                sibling = take();
                split(*current, *sibling);
            }
        }
        if (sibling) {
            auto root = take();
            root->leaf = false;
            root->children.push_back(std::move(root_));
            root->children.push_back(std::move(sibling));
            refresh(*root);
            root_ = std::move(root);
        }
        return {iterator(result_leaf, index), true};
    }

    static void split(Node& node, Node& right) noexcept {
        right.leaf = node.leaf;
        if (node.leaf) {
            const size_type middle = node.entries.size() / 2;
            right.entries.insert(right.entries.end(),
                std::make_move_iterator(node.entries.begin() + middle),
                std::make_move_iterator(node.entries.end()));
            node.entries.resize(middle);
            right.next = node.next;
            node.next = &right;
        } else {
            const size_type middle = node.children.size() / 2;
            right.children.insert(right.children.end(),
                std::make_move_iterator(node.children.begin() + middle),
                std::make_move_iterator(node.children.end()));
            node.children.resize(middle);
        }
        refresh(node);
        refresh(right);
    }
    static size_type occupancy(const Node& node) noexcept {
        return node.leaf ? node.entries.size() : node.children.size();
    }
    static void rebalance(Node& parent, size_type index) noexcept {
        Node& node = *parent.children[index];
        const size_type minimum_size = node.leaf ? Degree - 1 : Degree;
        if (occupancy(node) >= minimum_size) {
            return;
        }
        Node* left = index ? parent.children[index - 1].get() : nullptr;
        Node* right = index + 1 < parent.children.size() ? parent.children[index + 1].get() : nullptr;
        if (left && occupancy(*left) > minimum_size) {
            if (node.leaf) {
                node.entries.insert(node.entries.begin(), std::move(left->entries.back()));
                left->entries.pop_back();
            } else {
                node.children.insert(node.children.begin(), std::move(left->children.back()));
                left->children.pop_back();
            }
            refresh(*left);
            refresh(node);
        } else if (right && occupancy(*right) > minimum_size) {
            if (node.leaf) {
                node.entries.push_back(std::move(right->entries.front()));
                right->entries.erase(right->entries.begin());
            } else {
                node.children.push_back(std::move(right->children.front()));
                right->children.erase(right->children.begin());
            }
            refresh(node);
            refresh(*right);
        } else if (left || right) {
            const size_type left_index = left ? index - 1 : index;
            Node& destination = *parent.children[left_index];
            Node& source = *parent.children[left_index + 1];
            if (node.leaf) {
                destination.entries.insert(destination.entries.end(),
                    std::make_move_iterator(source.entries.begin()),
                    std::make_move_iterator(source.entries.end()));
                destination.next = source.next;
            } else {
                destination.children.insert(destination.children.end(),
                    std::make_move_iterator(source.children.begin()),
                    std::make_move_iterator(source.children.end()));
            }
            parent.children.erase(parent.children.begin() + left_index + 1);
            refresh(destination);
        }
    }
};

} // namespace data_structures

#endif
