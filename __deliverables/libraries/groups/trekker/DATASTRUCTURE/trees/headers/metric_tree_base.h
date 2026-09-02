#ifndef DATA_STRUCTURES_METRIC_TREE_BASE_H
#define DATA_STRUCTURES_METRIC_TREE_BASE_H

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace data_structures {

template <typename Item>
struct MetricTreeQueryResult {
    std::size_t index;
    Item item;
    double distance;
};

template <typename Item>
class MetricTreeBase {
public:
    using DistanceFunction = std::function<double(const Item&, const Item&)>;
    using QueryResult = MetricTreeQueryResult<Item>;

    explicit MetricTreeBase(DistanceFunction distance_function)
        : distance_function_(std::move(distance_function)) {}

    virtual ~MetricTreeBase() = default;

    virtual void build(const std::vector<Item>& items) = 0;
    virtual void insert(const Item& item) = 0;
    virtual void clear() = 0;
    virtual std::vector<QueryResult> k_nearest(const Item& query, std::size_t k) const = 0;
    virtual std::vector<QueryResult> radius_search(const Item& query, double radius) const = 0;

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

protected:
    DistanceFunction distance_function_;
    std::size_t size_ = 0;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_METRIC_TREE_BASE_H