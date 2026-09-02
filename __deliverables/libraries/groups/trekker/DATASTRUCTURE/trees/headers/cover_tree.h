#ifndef DATA_STRUCTURES_COVER_TREE_H
#define DATA_STRUCTURES_COVER_TREE_H

#include "metric_tree_base.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace data_structures {

template <typename Item>
class CoverTree : public MetricTreeBase<Item> {
public:
    using Base = MetricTreeBase<Item>;
    using typename Base::DistanceFunction;
    using typename Base::QueryResult;

    explicit CoverTree(DistanceFunction distance_function)
        : Base(std::move(distance_function)) {}

    void build(const std::vector<Item>& items) override {
        std::vector<std::size_t> order(items.size());
        for (std::size_t index = 0; index < items.size(); ++index) {
            order[index] = index;
        }
        rebuild_with_order(items, order);
    }

    void insert(const Item& item) override {
        items_.push_back(item);
        this->size_ = items_.size();
        insert_index(items_.size() - 1);
        if (root_) {
            recompute_metadata(root_);
        }
    }

    void clear() override {
        items_.clear();
        root_.reset();
        this->size_ = 0;
    }

    bool validate_invariants() const {
        if (!root_) {
            return true;
        }

        std::map<int, std::vector<std::size_t>> nodes_by_level;
        if (!validate_node(root_, nodes_by_level)) {
            return false;
        }

        for (const auto& [level, indices] : nodes_by_level) {
            for (std::size_t lhs = 0; lhs < indices.size(); ++lhs) {
                for (std::size_t rhs = lhs + 1; rhs < indices.size(); ++rhs) {
                    if (distance_between_indices(indices[lhs], indices[rhs]) <= cover_radius(level)) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    std::vector<QueryResult> k_nearest(const Item& query, std::size_t k) const override {
        if (!root_ || k == 0) {
            return {};
        }

        std::vector<QueryResult> results;
        results.reserve(std::min(k, items_.size()));
        k_nearest_recursive(root_, query, k, false, results);
        std::stable_sort(results.begin(), results.end(), result_less);
        return results;
    }

    std::vector<QueryResult> radius_search(const Item& query, double radius) const override {
        if (!root_) {
            return {};
        }

        std::vector<QueryResult> results;
        radius_search_recursive(root_, query, radius, false, results);
        std::stable_sort(results.begin(), results.end(), result_less);
        return results;
    }

protected:
    struct Node {
        std::size_t index;
        int level;
        double max_descendant_distance;
        std::vector<std::shared_ptr<Node>> children;

        Node(std::size_t node_index, int node_level)
            : index(node_index), level(node_level), max_descendant_distance(0.0) {}
    };

    const std::vector<Item>& items() const { return items_; }
    const std::shared_ptr<Node>& root() const { return root_; }

    std::vector<QueryResult> compute_all_distances(const Item& query) const {
        std::vector<QueryResult> results;
        results.reserve(items_.size());

        for (std::size_t index = 0; index < items_.size(); ++index) {
            results.push_back(QueryResult{index, items_[index], this->distance_function_(query, items_[index])});
        }

        std::stable_sort(results.begin(), results.end(), result_less);
        return results;
    }

    std::vector<QueryResult> collect_k_nearest(
        const Item& query,
        std::size_t k,
        bool order_children_by_query_distance) const
    {
        if (!root_ || k == 0) {
            return {};
        }

        std::vector<QueryResult> results;
        results.reserve(std::min(k, items_.size()));
        k_nearest_recursive(root_, query, k, order_children_by_query_distance, results);
        std::stable_sort(results.begin(), results.end(), result_less);
        return results;
    }

    std::vector<QueryResult> collect_radius_search(
        const Item& query,
        double radius,
        bool order_children_by_query_distance) const
    {
        if (!root_) {
            return {};
        }

        std::vector<QueryResult> results;
        radius_search_recursive(root_, query, radius, order_children_by_query_distance, results);
        std::stable_sort(results.begin(), results.end(), result_less);
        return results;
    }

    void build_from_index_order(const std::vector<std::size_t>& order) {
        if (items_.empty() || order.empty()) {
            root_.reset();
            return;
        }

        root_.reset();
        root_ = std::make_shared<Node>(order.front(), initial_root_level(items_));
        for (std::size_t order_index = 1; order_index < order.size(); ++order_index) {
            const std::size_t index = order[order_index];
            insert_index(index);
        }
        if (root_) {
            recompute_metadata(root_);
        }
    }

    void rebuild_with_order(const std::vector<Item>& items, const std::vector<std::size_t>& order) {
        items_ = items;
        this->size_ = items_.size();
        build_from_index_order(order);
    }

    static bool result_less(const QueryResult& lhs, const QueryResult& rhs) {
        if (lhs.distance == rhs.distance) {
            return lhs.index < rhs.index;
        }
        return lhs.distance < rhs.distance;
    }

private:
    using ChildDistancePair = std::pair<double, std::shared_ptr<Node>>;

    double distance_between_indices(std::size_t lhs, std::size_t rhs) const {
        return this->distance_function_(items_[lhs], items_[rhs]);
    }

    int initial_root_level(const std::vector<Item>& items) const {
        if (items.size() < 2) {
            return 0;
        }

        double diameter = 0.0;
        for (std::size_t lhs = 0; lhs < items.size(); ++lhs) {
            for (std::size_t rhs = lhs + 1; rhs < items.size(); ++rhs) {
                diameter = std::max(diameter, this->distance_function_(items[lhs], items[rhs]));
            }
        }

        if (diameter <= 1.0) {
            return 0;
        }

        return static_cast<int>(std::ceil(std::log2(diameter)));
    }

    double cover_radius(int level) const {
        return std::pow(2.0, static_cast<double>(level));
    }

    void insert_index(std::size_t index) {
        if (!root_) {
            root_ = std::make_shared<Node>(index, 0);
            return;
        }

        while (distance_between_indices(root_->index, index) > cover_radius(root_->level)) {
            auto promoted_root = std::make_shared<Node>(root_->index, root_->level + 1);
            promoted_root->children.push_back(root_);
            root_ = promoted_root;
        }

        insert_recursive(root_, index);
    }

    void insert_recursive(const std::shared_ptr<Node>& node, std::size_t index) {
        std::shared_ptr<Node> best_child;
        double best_distance = std::numeric_limits<double>::infinity();

        for (const auto& child : node->children) {
            const double child_distance = distance_between_indices(child->index, index);
            if (child_distance <= cover_radius(child->level) && child_distance < best_distance) {
                best_distance = child_distance;
                best_child = child;
            }
        }

        if (best_child) {
            insert_recursive(best_child, index);
            return;
        }

        node->children.push_back(std::make_shared<Node>(index, node->level - 1));
    }

    double recompute_metadata(const std::shared_ptr<Node>& node) const {
        double max_distance = 0.0;
        for (const auto& child : node->children) {
            const double child_extent = recompute_metadata(child);
            const double child_distance = distance_between_indices(node->index, child->index);
            max_distance = std::max(max_distance, child_distance + child_extent);
        }
        node->max_descendant_distance = max_distance;
        return max_distance;
    }

    void consider_candidate(
        std::size_t index,
        double distance,
        std::size_t k,
        std::vector<QueryResult>& results) const
    {
        for (auto& result : results) {
            if (result.index == index) {
                result.distance = std::min(result.distance, distance);
                std::stable_sort(results.begin(), results.end(), result_less);
                return;
            }
        }

        results.push_back(QueryResult{index, items_[index], distance});
        std::stable_sort(results.begin(), results.end(), result_less);
        if (results.size() > k) {
            results.resize(k);
        }
    }

    double current_worst_distance(std::size_t k, const std::vector<QueryResult>& results) const {
        if (results.size() < k) {
            return std::numeric_limits<double>::infinity();
        }
        return results.back().distance;
    }

    std::vector<ChildDistancePair> ordered_children(
        const std::shared_ptr<Node>& node,
        const Item& query,
        bool order_children_by_query_distance) const
    {
        std::vector<ChildDistancePair> children;
        children.reserve(node->children.size());
        for (const auto& child : node->children) {
            children.emplace_back(this->distance_function_(query, items_[child->index]), child);
        }

        if (order_children_by_query_distance) {
            std::stable_sort(children.begin(), children.end(), [](const ChildDistancePair& lhs, const ChildDistancePair& rhs) {
                return lhs.first < rhs.first;
            });
        }

        return children;
    }

    void k_nearest_recursive(
        const std::shared_ptr<Node>& node,
        const Item& query,
        std::size_t k,
        bool order_children_by_query_distance,
        std::vector<QueryResult>& results) const
    {
        const double node_distance = this->distance_function_(query, items_[node->index]);
        consider_candidate(node->index, node_distance, k, results);

        for (const auto& child_entry : ordered_children(node, query, order_children_by_query_distance)) {
            const double child_distance = child_entry.first;
            const auto& child = child_entry.second;
            const double min_possible_distance = std::max(0.0, child_distance - child->max_descendant_distance);
            if (min_possible_distance > current_worst_distance(k, results)) {
                continue;
            }
            k_nearest_recursive(child, query, k, order_children_by_query_distance, results);
        }
    }

    void radius_search_recursive(
        const std::shared_ptr<Node>& node,
        const Item& query,
        double radius,
        bool order_children_by_query_distance,
        std::vector<QueryResult>& results) const
    {
        const double node_distance = this->distance_function_(query, items_[node->index]);
        if (node_distance <= radius) {
            bool found = false;
            for (auto& result : results) {
                if (result.index == node->index) {
                    result.distance = std::min(result.distance, node_distance);
                    found = true;
                    break;
                }
            }

            if (!found) {
                results.push_back(QueryResult{node->index, items_[node->index], node_distance});
            }
        }

        for (const auto& child_entry : ordered_children(node, query, order_children_by_query_distance)) {
            const double child_distance = child_entry.first;
            const auto& child = child_entry.second;
            const double min_possible_distance = std::max(0.0, child_distance - child->max_descendant_distance);
            if (min_possible_distance > radius) {
                continue;
            }
            radius_search_recursive(child, query, radius, order_children_by_query_distance, results);
        }
    }

    bool validate_node(
        const std::shared_ptr<Node>& node,
        std::map<int, std::vector<std::size_t>>& nodes_by_level) const
    {
        nodes_by_level[node->level].push_back(node->index);

        for (const auto& child : node->children) {
            if (child->level != node->level - 1) {
                return false;
            }

            if (distance_between_indices(node->index, child->index) > cover_radius(node->level)) {
                return false;
            }

            if (!validate_node(child, nodes_by_level)) {
                return false;
            }
        }

        return true;
    }

    std::vector<Item> items_;
    std::shared_ptr<Node> root_;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_COVER_TREE_H