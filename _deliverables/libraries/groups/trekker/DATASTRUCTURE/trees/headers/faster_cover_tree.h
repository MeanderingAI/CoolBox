#ifndef DATA_STRUCTURES_FASTER_COVER_TREE_H
#define DATA_STRUCTURES_FASTER_COVER_TREE_H

#include "cover_tree.h"

#include <algorithm>
#include <vector>

namespace data_structures {

template <typename Item>
class FasterCoverTree : public CoverTree<Item> {
public:
    using Base = CoverTree<Item>;
    using typename Base::DistanceFunction;
    using typename Base::QueryResult;

    explicit FasterCoverTree(DistanceFunction distance_function)
        : Base(std::move(distance_function)) {}

    void build(const std::vector<Item>& items) override {
        buffered_items_ = items;
        auto order = farthest_first_order(buffered_items_);
        this->rebuild_with_order(buffered_items_, order);
    }

    void insert(const Item& item) override {
        buffered_items_.push_back(item);
        Base::insert(item);
    }

    void clear() override {
        buffered_items_.clear();
        Base::clear();
    }

    std::vector<QueryResult> k_nearest(const Item& query, std::size_t k) const override {
        return Base::collect_k_nearest(query, k, true);
    }

    std::vector<QueryResult> radius_search(const Item& query, double radius) const override {
        return Base::collect_radius_search(query, radius, true);
    }

private:
    std::vector<std::size_t> farthest_first_order(const std::vector<Item>& items) const {
        std::vector<std::size_t> order;
        if (items.empty()) {
            return order;
        }

        order.reserve(items.size());
        std::vector<bool> selected(items.size(), false);
        order.push_back(0);
        selected[0] = true;

        while (order.size() < items.size()) {
            std::size_t best_index = 0;
            double best_score = -1.0;

            for (std::size_t candidate = 0; candidate < items.size(); ++candidate) {
                if (selected[candidate]) {
                    continue;
                }

                double nearest_selected = std::numeric_limits<double>::infinity();
                for (std::size_t chosen : order) {
                    nearest_selected = std::min(
                        nearest_selected,
                        this->distance_function_(items[candidate], items[chosen]));
                }

                if (nearest_selected > best_score) {
                    best_score = nearest_selected;
                    best_index = candidate;
                }
            }

            selected[best_index] = true;
            order.push_back(best_index);
        }

        return order;
    }
    std::vector<Item> buffered_items_;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_FASTER_COVER_TREE_H