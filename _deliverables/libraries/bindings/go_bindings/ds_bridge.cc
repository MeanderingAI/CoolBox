#include "ds_bridge.h"

#include "binary_search_tree.h"
#include "btree.h"
#include "bloom_filter.h"
#include "concurrent_hash_map.h"
#include "concurrent_linked_list.h"
#include "heap.h"
#include "linked_list.h"
#include "log_structured_merge_tree.h"
#include "maple_tree.h"
#include "priority_queue.h"
#include "folio_queue.h"
#include "radix_tree.h"
#include "cover_tree.h"
#include "faster_cover_tree.h"
#include "red_black_tree.h"
#include "splay_tree.h"
#include "van_emde_boas.h"
#include "egh_filter.h"
#include "xarray.h"
#include "hash_map.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

using DSInt = std::int64_t;
using DSBinaryHeap = data_structures::BinaryHeap<DSInt>;
using DSMinBinaryHeap = data_structures::BinaryHeap<DSInt, std::greater<DSInt>>;
using DSIndexedQueue = data_structures::IndexedPriorityQueue<DSInt, double>;
using DSMap = data_structures::HashMap<DSInt, DSInt>;
using DSConcurrentMap = data_structures::ConcurrentHashMap<DSInt, DSInt>;
using DSBPlusTree = data_structures::BPlusTree<DSInt, DSInt, 4>;
using DSLsmTree = data_structures::LogStructuredMergeTree<DSInt, DSInt>;
using DSMetricItem = std::vector<double>;
using DSCoverTree = data_structures::CoverTree<DSMetricItem>;
using DSFasterCoverTree = data_structures::FasterCoverTree<DSMetricItem>;
using DSCoverTreeVariant = std::variant<DSCoverTree, DSFasterCoverTree>;
using DSTreeVariant = std::variant<
    data_structures::BinarySearchTree<DSInt>,
    data_structures::RedBlackTree<DSInt>,
    data_structures::SplayTree<DSInt>>;
using DSFilterVariant = std::variant<
    data_structures::BloomFilter<DSInt>,
    data_structures::EGHFilter<DSInt>>;
using DSArrayVariant = std::variant<
    xarray<double, 1>, xarray<double, 2>, xarray<double, 3>, xarray<double, 4>>;

namespace {

template <typename F>
int status_call(F&& fn) noexcept {
    try {
        fn();
        return 1;
    } catch (...) {
        return 0;
    }
}

struct IntList {
    data_structures::LinkedList<DSInt> value;
};

struct IntHeap {
    bool min_heap;
    DSBinaryHeap max;
    DSMinBinaryHeap min;

    explicit IntHeap(bool use_min) : min_heap(use_min), max(), min() {}
};

struct IndexedQueue {
    DSIndexedQueue value;
};

struct IntMap {
    bool concurrent;
    DSMap map;
    DSConcurrentMap concurrent_map;

    IntMap(bool use_concurrent, size_t capacity)
        : concurrent(use_concurrent), map(capacity), concurrent_map(capacity) {}
};

struct IntTree {
    DSTreeVariant value;
    explicit IntTree(int kind) : value(kind == 1
        ? DSTreeVariant(std::in_place_type<data_structures::RedBlackTree<DSInt>>)
        : kind == 2
            ? DSTreeVariant(std::in_place_type<data_structures::SplayTree<DSInt>>)
            : DSTreeVariant(std::in_place_type<data_structures::BinarySearchTree<DSInt>>)) {}
};

struct IntFilter {
    DSFilterVariant value;
    explicit IntFilter(int kind, size_t expected, double rate) : value(kind == 1
        ? DSFilterVariant(std::in_place_type<data_structures::EGHFilter<DSInt>>, expected, rate)
        : DSFilterVariant(std::in_place_type<data_structures::BloomFilter<DSInt>>, expected, rate)) {}
};

template <size_t N>
xarray<double, N> make_xarray(const size_t* shape) {
    typename xarray<double, N>::shape_type dimensions{};
    std::copy(shape, shape + N, dimensions.begin());
    return xarray<double, N>(dimensions);
}

struct FloatArray {
    size_t rank;
    std::vector<size_t> shape;
    DSArrayVariant value;

    FloatArray(size_t dimensions, const size_t* dimensions_shape)
        : rank(dimensions), shape(dimensions_shape, dimensions_shape + dimensions),
          value(make_array(dimensions, dimensions_shape)) {}

    static DSArrayVariant make_array(size_t rank, const size_t* shape) {
        if (rank == 1) return make_xarray<1>(shape);
        if (rank == 2) return make_xarray<2>(shape);
        if (rank == 3) return make_xarray<3>(shape);
        return make_xarray<4>(shape);
    }
};

struct IntQueue {
    folio_queue<DSInt> value;
};

struct ConcurrentIntList {
    data_structures::ConcurrentLinkedList<DSInt> value;
};

struct IntBPlusTree {
    DSBPlusTree value;
};

struct IntRadixTree {
    radix_tree<DSInt> value;
};

struct DoublyIntList {
    data_structures::DoublyLinkedList<DSInt> value;
};

struct LegacyIntHeap {
    bool min_heap;
    data_structures::Heap<DSInt> max;
    data_structures::Heap<DSInt, std::greater<DSInt>> min;
    explicit LegacyIntHeap(bool use_min) : min_heap(use_min), max(), min(std::greater<DSInt>{}) {}
};

struct IntLsmTree {
    DSLsmTree value;
    explicit IntLsmTree(size_t threshold) : value(threshold) {}
};

struct IntMapleTree {
    MapleTree<DSInt, DSInt, 4> value;
};

struct MetricTree {
    size_t dimensions;
    DSCoverTreeVariant value;
    MetricTree(bool faster, size_t dimensions_count)
        : dimensions(dimensions_count),
          value(faster
              ? DSCoverTreeVariant(std::in_place_type<DSFasterCoverTree>, euclidean_distance)
              : DSCoverTreeVariant(std::in_place_type<DSCoverTree>, euclidean_distance)) {}

    static double euclidean_distance(const DSMetricItem& lhs, const DSMetricItem& rhs) {
        if (lhs.size() != rhs.size()) {
            return std::numeric_limits<double>::infinity();
        }
        double distance = 0.0;
        for (size_t i = 0; i < lhs.size(); ++i) {
            const double delta = lhs[i] - rhs[i];
            distance = std::hypot(distance, delta);
        }
        return distance;
    }
};

template <typename Tree>
std::vector<DSInt> tree_values(const Tree& tree) {
    std::vector<DSInt> values;
    tree.inorder_traversal([&values](const DSInt& value) { values.push_back(value); });
    return values;
}

template <size_t N>
std::array<size_t, N> fixed_indices(const size_t* indices) {
    std::array<size_t, N> result{};
    std::copy(indices, indices + N, result.begin());
    return result;
}

} // namespace

extern "C" {

void* coolbox_ds_list_new(void) {
    try { return new IntList(); } catch (...) { return nullptr; }
}
void coolbox_ds_list_free(void* handle) { delete static_cast<IntList*>(handle); }
int coolbox_ds_list_push_front(void* handle, int64_t value) {
    return handle ? status_call([&] { static_cast<IntList*>(handle)->value.push_front(value); }) : 0;
}
int coolbox_ds_list_push_back(void* handle, int64_t value) {
    return handle ? status_call([&] { static_cast<IntList*>(handle)->value.push_back(value); }) : 0;
}
int coolbox_ds_list_insert(void* handle, size_t index, int64_t value) {
    return handle ? status_call([&] { static_cast<IntList*>(handle)->value.insert_at(index, value); }) : 0;
}
int coolbox_ds_list_pop_front(void* handle, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<IntList*>(handle)->value.pop_front(); });
}
int coolbox_ds_list_pop_back(void* handle, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<IntList*>(handle)->value.pop_back(); });
}
int coolbox_ds_list_remove_at(void* handle, size_t index, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<IntList*>(handle)->value.remove_at(index); });
}
int coolbox_ds_list_remove_value(void* handle, int64_t value, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<IntList*>(handle)->value.remove_value(value); });
}
int coolbox_ds_list_at(const void* handle, size_t index, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] { *value = static_cast<const IntList*>(handle)->value.at(index); });
}
int coolbox_ds_list_front(const void* handle, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] { *value = static_cast<const IntList*>(handle)->value.front(); });
}
int coolbox_ds_list_back(const void* handle, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] { *value = static_cast<const IntList*>(handle)->value.back(); });
}
int coolbox_ds_list_find(const void* handle, int64_t value) {
    return handle && static_cast<const IntList*>(handle)->value.find(value);
}
size_t coolbox_ds_list_size(const void* handle) {
    return handle ? static_cast<const IntList*>(handle)->value.size() : 0;
}
int coolbox_ds_list_empty(const void* handle) {
    return !handle || static_cast<const IntList*>(handle)->value.empty();
}
int coolbox_ds_list_clear(void* handle) {
    return handle ? status_call([&] { static_cast<IntList*>(handle)->value.clear(); }) : 0;
}
int coolbox_ds_list_reverse(void* handle) {
    return handle ? status_call([&] { static_cast<IntList*>(handle)->value.reverse(); }) : 0;
}
size_t coolbox_ds_list_values(const void* handle, int64_t* values, size_t capacity) {
    if (!handle) return 0;
    std::vector<DSInt> copy;
    static_cast<const IntList*>(handle)->value.for_each([&copy](const DSInt& item) { copy.push_back(item); });
    if (values) std::copy_n(copy.begin(), std::min(copy.size(), capacity), values);
    return copy.size();
}

void* coolbox_ds_heap_new(int min_heap) {
    try { return new IntHeap(min_heap != 0); } catch (...) { return nullptr; }
}
void coolbox_ds_heap_free(void* handle) { delete static_cast<IntHeap*>(handle); }
int coolbox_ds_heap_push(void* handle, int64_t value) {
    if (!handle) return 0;
    return status_call([&] {
        auto& heap = *static_cast<IntHeap*>(handle);
        if (heap.min_heap) heap.min.push(value); else heap.max.push(value);
    });
}
int coolbox_ds_heap_pop(void* handle) {
    if (!handle) return 0;
    return status_call([&] {
        auto& heap = *static_cast<IntHeap*>(handle);
        if (heap.min_heap) heap.min.pop(); else heap.max.pop();
    });
}
int coolbox_ds_heap_top(const void* handle, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] {
        const auto& heap = *static_cast<const IntHeap*>(handle);
        *value = heap.min_heap ? heap.min.top() : heap.max.top();
    });
}
size_t coolbox_ds_heap_size(const void* handle) {
    if (!handle) return 0;
    const auto& heap = *static_cast<const IntHeap*>(handle);
    return heap.min_heap ? heap.min.size() : heap.max.size();
}
int coolbox_ds_heap_empty(const void* handle) {
    if (!handle) return 1;
    const auto& heap = *static_cast<const IntHeap*>(handle);
    return heap.min_heap ? heap.min.empty() : heap.max.empty();
}
int coolbox_ds_heap_clear(void* handle) {
    if (!handle) return 0;
    return status_call([&] {
        auto& heap = *static_cast<IntHeap*>(handle);
        if (heap.min_heap) heap.min.clear(); else heap.max.clear();
    });
}

void* coolbox_ds_indexed_queue_new(void) {
    try { return new IndexedQueue(); } catch (...) { return nullptr; }
}
void coolbox_ds_indexed_queue_free(void* handle) { delete static_cast<IndexedQueue*>(handle); }
int coolbox_ds_indexed_queue_push(void* handle, int64_t key, double priority) {
    return handle ? status_call([&] { static_cast<IndexedQueue*>(handle)->value.push(key, priority); }) : 0;
}
int coolbox_ds_indexed_queue_update(void* handle, int64_t key, double priority) {
    return handle ? status_call([&] { static_cast<IndexedQueue*>(handle)->value.update(key, priority); }) : 0;
}
int coolbox_ds_indexed_queue_pop(void* handle) {
    return handle ? status_call([&] { static_cast<IndexedQueue*>(handle)->value.pop(); }) : 0;
}
int coolbox_ds_indexed_queue_top(const void* handle, int64_t* key, double* priority) {
    if (!handle || !key || !priority) return 0;
    return status_call([&] {
        const auto top = static_cast<const IndexedQueue*>(handle)->value.top();
        *key = top.first; *priority = top.second;
    });
}
int coolbox_ds_indexed_queue_contains(const void* handle, int64_t key) {
    return handle && static_cast<const IndexedQueue*>(handle)->value.contains(key);
}
size_t coolbox_ds_indexed_queue_size(const void* handle) {
    return handle ? static_cast<const IndexedQueue*>(handle)->value.size() : 0;
}
int coolbox_ds_indexed_queue_empty(const void* handle) {
    return !handle || static_cast<const IndexedQueue*>(handle)->value.empty();
}
int coolbox_ds_indexed_queue_clear(void* handle) {
    return handle ? status_call([&] { static_cast<IndexedQueue*>(handle)->value.clear(); }) : 0;
}

void* coolbox_ds_map_new(int concurrent, size_t initial_capacity) {
    try { return new IntMap(concurrent != 0, initial_capacity); } catch (...) { return nullptr; }
}
void coolbox_ds_map_free(void* handle) { delete static_cast<IntMap*>(handle); }
int coolbox_ds_map_insert(void* handle, int64_t key, int64_t value) {
    if (!handle) return 0;
    return status_call([&] {
        auto& map = *static_cast<IntMap*>(handle);
        if (map.concurrent) map.concurrent_map.insert(key, value); else map.map.insert(key, value);
    });
}
int coolbox_ds_map_remove(void* handle, int64_t key, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] {
        auto& map = *static_cast<IntMap*>(handle);
        *removed = map.concurrent ? map.concurrent_map.remove(key) : map.map.remove(key);
    });
}
int coolbox_ds_map_get(const void* handle, int64_t key, int64_t* value, int* found) {
    if (!handle || !value || !found) return 0;
    return status_call([&] {
        const auto& map = *static_cast<const IntMap*>(handle);
        *found = map.concurrent ? map.concurrent_map.get(key, *value) : map.map.get(key, *value);
    });
}
int coolbox_ds_map_contains(const void* handle, int64_t key) {
    if (!handle) return 0;
    const auto& map = *static_cast<const IntMap*>(handle);
    return map.concurrent ? map.concurrent_map.contains(key) : map.map.contains(key);
}
size_t coolbox_ds_map_size(const void* handle) {
    if (!handle) return 0;
    const auto& map = *static_cast<const IntMap*>(handle);
    return map.concurrent ? map.concurrent_map.size() : map.map.size();
}
int coolbox_ds_map_clear(void* handle) {
    if (!handle) return 0;
    return status_call([&] {
        auto& map = *static_cast<IntMap*>(handle);
        if (map.concurrent) map.concurrent_map.clear(); else map.map.clear();
    });
}

void* coolbox_ds_tree_new(int kind) {
    if (kind < 0 || kind > 2) return nullptr;
    try { return new IntTree(kind); } catch (...) { return nullptr; }
}
void coolbox_ds_tree_free(void* handle) { delete static_cast<IntTree*>(handle); }
int coolbox_ds_tree_insert(void* handle, int64_t value) {
    return handle ? status_call([&] {
        std::visit([&](auto& tree) { tree.insert(value); }, static_cast<IntTree*>(handle)->value);
    }) : 0;
}
int coolbox_ds_tree_remove(void* handle, int64_t value, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] {
        *removed = std::visit([&](auto& tree) { return tree.remove(value); }, static_cast<IntTree*>(handle)->value);
    });
}
int coolbox_ds_tree_contains(void* handle, int64_t value) {
    if (!handle) return 0;
    return std::visit([&](auto& tree) { return tree.search(value); }, static_cast<IntTree*>(handle)->value);
}
size_t coolbox_ds_tree_size(const void* handle) {
    return handle ? std::visit([](const auto& tree) { return tree.size(); }, static_cast<const IntTree*>(handle)->value) : 0;
}
int coolbox_ds_tree_empty(const void* handle) {
    return !handle || std::visit([](const auto& tree) { return tree.empty(); }, static_cast<const IntTree*>(handle)->value);
}
int coolbox_ds_tree_clear(void* handle) {
    return handle ? status_call([&] {
        std::visit([](auto& tree) { tree.clear(); }, static_cast<IntTree*>(handle)->value);
    }) : 0;
}
int coolbox_ds_tree_min(const void* handle, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] {
        const auto& tree = static_cast<const IntTree*>(handle)->value;
        *value = std::visit([](const auto& current) -> DSInt {
            using T = std::decay_t<decltype(current)>;
            if constexpr (std::is_same_v<T, data_structures::SplayTree<DSInt>>) {
                auto values = tree_values(current);
                if (values.empty()) throw std::runtime_error("empty tree");
                return values.front();
            } else {
                return current.min();
            }
        }, tree);
    });
}
int coolbox_ds_tree_max(const void* handle, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] {
        const auto& tree = static_cast<const IntTree*>(handle)->value;
        *value = std::visit([](const auto& current) -> DSInt {
            using T = std::decay_t<decltype(current)>;
            if constexpr (std::is_same_v<T, data_structures::SplayTree<DSInt>>) {
                auto values = tree_values(current);
                if (values.empty()) throw std::runtime_error("empty tree");
                return values.back();
            } else {
                return current.max();
            }
        }, tree);
    });
}
size_t coolbox_ds_tree_values(const void* handle, int64_t* values, size_t capacity) {
    if (!handle) return 0;
    const auto copy = std::visit([](const auto& tree) { return tree_values(tree); }, static_cast<const IntTree*>(handle)->value);
    if (values) std::copy_n(copy.begin(), std::min(copy.size(), capacity), values);
    return copy.size();
}

void* coolbox_ds_filter_new(int kind, size_t expected_insertions, double false_positive_rate) {
    if (kind < 0 || kind > 1 || expected_insertions == 0 ||
        !(false_positive_rate > 0.0 && false_positive_rate < 1.0)) return nullptr;
    try { return new IntFilter(kind, expected_insertions, false_positive_rate); } catch (...) { return nullptr; }
}
void coolbox_ds_filter_free(void* handle) { delete static_cast<IntFilter*>(handle); }
int coolbox_ds_filter_insert(void* handle, int64_t value) {
    return handle ? status_call([&] {
        std::visit([&](auto& filter) { filter.insert(value); }, static_cast<IntFilter*>(handle)->value);
    }) : 0;
}
int coolbox_ds_filter_contains(const void* handle, int64_t value) {
    return handle && std::visit([&](const auto& filter) { return filter.contains(value); }, static_cast<const IntFilter*>(handle)->value);
}
size_t coolbox_ds_filter_insert_count(const void* handle) {
    return handle ? std::visit([](const auto& filter) { return filter.insert_count(); }, static_cast<const IntFilter*>(handle)->value) : 0;
}
size_t coolbox_ds_filter_approximate_count(const void* handle) {
    return handle ? std::visit([](const auto& filter) { return filter.approximate_count(); }, static_cast<const IntFilter*>(handle)->value) : 0;
}
double coolbox_ds_filter_estimated_rate(const void* handle) {
    return handle ? std::visit([](const auto& filter) { return filter.estimated_false_positive_rate(); }, static_cast<const IntFilter*>(handle)->value) : 0.0;
}
int coolbox_ds_filter_clear(void* handle) {
    return handle ? status_call([&] {
        std::visit([](auto& filter) { filter.clear(); }, static_cast<IntFilter*>(handle)->value);
    }) : 0;
}

void* coolbox_ds_veb_new(uint64_t universe_size) {
    try { return new data_structures::VanEmdeBoasTree(universe_size); } catch (...) { return nullptr; }
}
void coolbox_ds_veb_free(void* handle) { delete static_cast<data_structures::VanEmdeBoasTree*>(handle); }
int coolbox_ds_veb_insert(void* handle, uint64_t value) {
    return handle ? status_call([&] { static_cast<data_structures::VanEmdeBoasTree*>(handle)->insert(value); }) : 0;
}
int coolbox_ds_veb_remove(void* handle, uint64_t value) {
    return handle ? status_call([&] { static_cast<data_structures::VanEmdeBoasTree*>(handle)->remove(value); }) : 0;
}
int coolbox_ds_veb_contains(const void* handle, uint64_t value) {
    return handle && static_cast<const data_structures::VanEmdeBoasTree*>(handle)->contains(value);
}
uint64_t coolbox_ds_veb_min(const void* handle) {
    return handle ? static_cast<const data_structures::VanEmdeBoasTree*>(handle)->minimum() : data_structures::VanEmdeBoasTree::NONE;
}
uint64_t coolbox_ds_veb_max(const void* handle) {
    return handle ? static_cast<const data_structures::VanEmdeBoasTree*>(handle)->maximum() : data_structures::VanEmdeBoasTree::NONE;
}
uint64_t coolbox_ds_veb_successor(const void* handle, uint64_t value) {
    return handle ? static_cast<const data_structures::VanEmdeBoasTree*>(handle)->successor(value) : data_structures::VanEmdeBoasTree::NONE;
}
uint64_t coolbox_ds_veb_predecessor(const void* handle, uint64_t value) {
    return handle ? static_cast<const data_structures::VanEmdeBoasTree*>(handle)->predecessor(value) : data_structures::VanEmdeBoasTree::NONE;
}
uint64_t coolbox_ds_veb_universe_size(const void* handle) {
    return handle ? static_cast<const data_structures::VanEmdeBoasTree*>(handle)->universe_size() : 0;
}
int coolbox_ds_veb_empty(const void* handle) {
    return !handle || static_cast<const data_structures::VanEmdeBoasTree*>(handle)->empty();
}

void* coolbox_ds_array_new(size_t rank, const size_t* shape) {
    if (!shape || rank < 1 || rank > 4) return nullptr;
    try {
        for (size_t i = 0; i < rank; ++i) if (shape[i] == 0) return nullptr;
        return new FloatArray(rank, shape);
    } catch (...) { return nullptr; }
}
void coolbox_ds_array_free(void* handle) { delete static_cast<FloatArray*>(handle); }
size_t coolbox_ds_array_rank(const void* handle) { return handle ? static_cast<const FloatArray*>(handle)->rank : 0; }
size_t coolbox_ds_array_size(const void* handle) {
    if (!handle) return 0;
    const auto& array = *static_cast<const FloatArray*>(handle);
    size_t size = 1;
    for (const auto dimension : array.shape) size *= dimension;
    return size;
}
int coolbox_ds_array_shape(const void* handle, size_t* shape, size_t capacity) {
    if (!handle || !shape || capacity < static_cast<const FloatArray*>(handle)->rank) return 0;
    const auto& dimensions = static_cast<const FloatArray*>(handle)->shape;
    std::copy(dimensions.begin(), dimensions.end(), shape);
    return 1;
}
int coolbox_ds_array_get(const void* handle, const size_t* indices, size_t rank, double* value) {
    if (!handle || !indices || !value || rank != static_cast<const FloatArray*>(handle)->rank) return 0;
    return status_call([&] {
        const auto& array = *static_cast<const FloatArray*>(handle);
        *value = std::visit([&](const auto& current) -> double {
            constexpr size_t N = std::tuple_size_v<typename std::decay_t<decltype(current)>::shape_type>;
            return current[fixed_indices<N>(indices)];
        }, array.value);
    });
}
int coolbox_ds_array_set(void* handle, const size_t* indices, size_t rank, double value) {
    if (!handle || !indices || rank != static_cast<FloatArray*>(handle)->rank) return 0;
    return status_call([&] {
        auto& array = *static_cast<FloatArray*>(handle);
        std::visit([&](auto& current) {
            constexpr size_t N = std::tuple_size_v<typename std::decay_t<decltype(current)>::shape_type>;
            current[fixed_indices<N>(indices)] = value;
        }, array.value);
    });
}

void* coolbox_ds_queue_new(void) {
    try { return new IntQueue(); } catch (...) { return nullptr; }
}
void coolbox_ds_queue_free(void* handle) { delete static_cast<IntQueue*>(handle); }
int coolbox_ds_queue_enqueue(void* handle, int64_t value) {
    return handle ? status_call([&] { static_cast<IntQueue*>(handle)->value.enqueue(value); }) : 0;
}
int coolbox_ds_queue_enqueue_batch(void* handle, const int64_t* values, size_t count) {
    if (!handle || (count > 0 && !values)) return 0;
    return status_call([&] {
        std::vector<DSInt> batch;
        if (count > 0) batch.assign(values, values + count);
        static_cast<IntQueue*>(handle)->value.enqueue_batch(batch);
    });
}
int coolbox_ds_queue_try_dequeue(void* handle, int64_t* value, int* found) {
    if (!handle || !value || !found) return 0;
    return status_call([&] {
        const auto result = static_cast<IntQueue*>(handle)->value.try_dequeue();
        *found = result.has_value();
        if (result) *value = *result;
    });
}
size_t coolbox_ds_queue_size(const void* handle) {
    return handle ? static_cast<const IntQueue*>(handle)->value.size() : 0;
}
int coolbox_ds_queue_empty(const void* handle) {
    return !handle || static_cast<const IntQueue*>(handle)->value.empty();
}

void* coolbox_ds_concurrent_list_new(void) {
    try { return new ConcurrentIntList(); } catch (...) { return nullptr; }
}
void coolbox_ds_concurrent_list_free(void* handle) { delete static_cast<ConcurrentIntList*>(handle); }
int coolbox_ds_concurrent_list_push_front(void* handle, int64_t value) {
    return handle ? status_call([&] { static_cast<ConcurrentIntList*>(handle)->value.push_front(value); }) : 0;
}
int coolbox_ds_concurrent_list_push_back(void* handle, int64_t value) {
    return handle ? status_call([&] { static_cast<ConcurrentIntList*>(handle)->value.push_back(value); }) : 0;
}
int coolbox_ds_concurrent_list_pop_front(void* handle, int64_t* value, int* found) {
    if (!handle || !value || !found) return 0;
    return status_call([&] { *found = static_cast<ConcurrentIntList*>(handle)->value.pop_front(*value); });
}
int coolbox_ds_concurrent_list_remove(void* handle, int64_t value, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<ConcurrentIntList*>(handle)->value.remove_value(value); });
}
int coolbox_ds_concurrent_list_contains(const void* handle, int64_t value) {
    return handle && static_cast<const ConcurrentIntList*>(handle)->value.find(value);
}
size_t coolbox_ds_concurrent_list_size(const void* handle) {
    return handle ? static_cast<const ConcurrentIntList*>(handle)->value.size() : 0;
}
int coolbox_ds_concurrent_list_empty(const void* handle) {
    return !handle || static_cast<const ConcurrentIntList*>(handle)->value.empty();
}

void* coolbox_ds_bplus_tree_new(void) {
    try { return new IntBPlusTree(); } catch (...) { return nullptr; }
}
void coolbox_ds_bplus_tree_free(void* handle) { delete static_cast<IntBPlusTree*>(handle); }
int coolbox_ds_bplus_tree_insert(void* handle, int64_t key, int64_t value) {
    return handle ? status_call([&] { static_cast<IntBPlusTree*>(handle)->value.insert(key, value); }) : 0;
}
int coolbox_ds_bplus_tree_get(const void* handle, int64_t key, int64_t* value, int* found) {
    if (!handle || !value || !found) return 0;
    return status_call([&] {
        const auto result = static_cast<const IntBPlusTree*>(handle)->value.search(key);
        *found = result.has_value();
        if (result) *value = *result;
    });
}
int coolbox_ds_bplus_tree_remove(void* handle, int64_t key, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<IntBPlusTree*>(handle)->value.remove(key); });
}
size_t coolbox_ds_bplus_tree_size(const void* handle) {
    return handle ? static_cast<const IntBPlusTree*>(handle)->value.size() : 0;
}
int coolbox_ds_bplus_tree_clear(void* handle) {
    return handle ? status_call([&] { static_cast<IntBPlusTree*>(handle)->value.clear(); }) : 0;
}
int coolbox_ds_bplus_tree_entries(const void* handle, int range, int64_t minimum, int64_t maximum,
                                  size_t* count, int64_t* keys, int64_t* values, size_t capacity) {
    if (!handle || !count || ((keys == nullptr) != (values == nullptr))) return 0;
    return status_call([&] {
        const auto& tree = static_cast<const IntBPlusTree*>(handle)->value;
        const auto entries = range ? tree.range_query(minimum, maximum) : tree.all_entries();
        *count = entries.size();
        if (keys && values) {
            const size_t copied = std::min(entries.size(), capacity);
            for (size_t i = 0; i < copied; ++i) {
                keys[i] = entries[i].first;
                values[i] = entries[i].second;
            }
        }
    });
}

void* coolbox_ds_radix_tree_new(void) {
    try { return new IntRadixTree(); } catch (...) { return nullptr; }
}
void coolbox_ds_radix_tree_free(void* handle) { delete static_cast<IntRadixTree*>(handle); }
int coolbox_ds_radix_tree_insert(void* handle, const char* key, int64_t value) {
    if (!handle || !key) return 0;
    return status_call([&] { static_cast<IntRadixTree*>(handle)->value.insert(key, value); });
}
int coolbox_ds_radix_tree_get(const void* handle, const char* key, int64_t* value, int* found) {
    if (!handle || !key || !value || !found) return 0;
    return status_call([&] {
        const auto result = static_cast<const IntRadixTree*>(handle)->value.find(key);
        *found = result.has_value();
        if (result) *value = *result;
    });
}
int coolbox_ds_radix_tree_remove(void* handle, const char* key, int* removed) {
    if (!handle || !key || !removed) return 0;
    return status_call([&] { *removed = static_cast<IntRadixTree*>(handle)->value.remove(key); });
}

void* coolbox_ds_doubly_list_new(void) {
    try { return new DoublyIntList(); } catch (...) { return nullptr; }
}
void coolbox_ds_doubly_list_free(void* handle) { delete static_cast<DoublyIntList*>(handle); }
int coolbox_ds_doubly_list_push_front(void* handle, int64_t value) {
    return handle ? status_call([&] { static_cast<DoublyIntList*>(handle)->value.push_front(value); }) : 0;
}
int coolbox_ds_doubly_list_push_back(void* handle, int64_t value) {
    return handle ? status_call([&] { static_cast<DoublyIntList*>(handle)->value.push_back(value); }) : 0;
}
int coolbox_ds_doubly_list_insert(void* handle, size_t index, int64_t value) {
    return handle ? status_call([&] { static_cast<DoublyIntList*>(handle)->value.insert_at(index, value); }) : 0;
}
int coolbox_ds_doubly_list_pop_front(void* handle, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<DoublyIntList*>(handle)->value.pop_front(); });
}
int coolbox_ds_doubly_list_pop_back(void* handle, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<DoublyIntList*>(handle)->value.pop_back(); });
}
int coolbox_ds_doubly_list_remove_at(void* handle, size_t index, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<DoublyIntList*>(handle)->value.remove_at(index); });
}
int coolbox_ds_doubly_list_front(const void* handle, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] { *value = static_cast<const DoublyIntList*>(handle)->value.front(); });
}
int coolbox_ds_doubly_list_back(const void* handle, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] { *value = static_cast<const DoublyIntList*>(handle)->value.back(); });
}
int coolbox_ds_doubly_list_at(const void* handle, size_t index, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] { *value = static_cast<const DoublyIntList*>(handle)->value.at(index); });
}
size_t coolbox_ds_doubly_list_size(const void* handle) {
    return handle ? static_cast<const DoublyIntList*>(handle)->value.size() : 0;
}
int coolbox_ds_doubly_list_empty(const void* handle) {
    return !handle || static_cast<const DoublyIntList*>(handle)->value.empty();
}
int coolbox_ds_doubly_list_clear(void* handle) {
    return handle ? status_call([&] { static_cast<DoublyIntList*>(handle)->value.clear(); }) : 0;
}

void* coolbox_ds_legacy_heap_new(int min_heap) {
    try { return new LegacyIntHeap(min_heap != 0); } catch (...) { return nullptr; }
}
void coolbox_ds_legacy_heap_free(void* handle) { delete static_cast<LegacyIntHeap*>(handle); }
int coolbox_ds_legacy_heap_push(void* handle, int64_t value) {
    if (!handle) return 0;
    return status_call([&] {
        auto& heap = *static_cast<LegacyIntHeap*>(handle);
        if (heap.min_heap) heap.min.push(value); else heap.max.push(value);
    });
}
int coolbox_ds_legacy_heap_pop(void* handle) {
    if (!handle) return 0;
    return status_call([&] {
        auto& heap = *static_cast<LegacyIntHeap*>(handle);
        const bool popped = heap.min_heap ? heap.min.pop() : heap.max.pop();
        if (!popped) throw std::underflow_error("Heap is empty");
    });
}
int coolbox_ds_legacy_heap_top(const void* handle, int64_t* value) {
    if (!handle || !value) return 0;
    return status_call([&] {
        const auto& heap = *static_cast<const LegacyIntHeap*>(handle);
        *value = heap.min_heap ? heap.min.top() : heap.max.top();
    });
}
size_t coolbox_ds_legacy_heap_size(const void* handle) {
    if (!handle) return 0;
    const auto& heap = *static_cast<const LegacyIntHeap*>(handle);
    return heap.min_heap ? heap.min.size() : heap.max.size();
}
int coolbox_ds_legacy_heap_empty(const void* handle) {
    if (!handle) return 1;
    const auto& heap = *static_cast<const LegacyIntHeap*>(handle);
    return heap.min_heap ? heap.min.empty() : heap.max.empty();
}
int coolbox_ds_legacy_heap_clear(void* handle) {
    if (!handle) return 0;
    return status_call([&] {
        auto& heap = *static_cast<LegacyIntHeap*>(handle);
        if (heap.min_heap) heap.min.clear(); else heap.max.clear();
    });
}

void* coolbox_ds_lsm_new(size_t threshold) {
    try { return new IntLsmTree(threshold); } catch (...) { return nullptr; }
}
void coolbox_ds_lsm_free(void* handle) { delete static_cast<IntLsmTree*>(handle); }
int coolbox_ds_lsm_put(void* handle, int64_t key, int64_t value) {
    return handle ? status_call([&] { static_cast<IntLsmTree*>(handle)->value.put(key, value); }) : 0;
}
int coolbox_ds_lsm_get(const void* handle, int64_t key, int64_t* value, int* found) {
    if (!handle || !value || !found) return 0;
    return status_call([&] { *found = static_cast<const IntLsmTree*>(handle)->value.get(key, *value); });
}
int coolbox_ds_lsm_remove(void* handle, int64_t key, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<IntLsmTree*>(handle)->value.remove(key); });
}
int coolbox_ds_lsm_contains(const void* handle, int64_t key) {
    return handle && static_cast<const IntLsmTree*>(handle)->value.contains(key);
}
int coolbox_ds_lsm_flush(void* handle) {
    return handle ? status_call([&] { static_cast<IntLsmTree*>(handle)->value.flush(); }) : 0;
}
int coolbox_ds_lsm_compact(void* handle) {
    return handle ? status_call([&] { static_cast<IntLsmTree*>(handle)->value.compact(); }) : 0;
}
int coolbox_ds_lsm_clear(void* handle) {
    return handle ? status_call([&] { static_cast<IntLsmTree*>(handle)->value.clear(); }) : 0;
}
size_t coolbox_ds_lsm_size(const void* handle) {
    return handle ? static_cast<const IntLsmTree*>(handle)->value.size() : 0;
}
size_t coolbox_ds_lsm_memtable_size(const void* handle) {
    return handle ? static_cast<const IntLsmTree*>(handle)->value.memtable_size() : 0;
}
size_t coolbox_ds_lsm_sstable_count(const void* handle) {
    return handle ? static_cast<const IntLsmTree*>(handle)->value.sstable_count() : 0;
}

void* coolbox_ds_maple_new(void) {
    try { return new IntMapleTree(); } catch (...) { return nullptr; }
}
void coolbox_ds_maple_free(void* handle) { delete static_cast<IntMapleTree*>(handle); }
int coolbox_ds_maple_insert(void* handle, int64_t key, int64_t value) {
    return handle ? status_call([&] { static_cast<IntMapleTree*>(handle)->value.insert(key, value); }) : 0;
}
int coolbox_ds_maple_get(const void* handle, int64_t key, int64_t* value, int* found) {
    if (!handle || !value || !found) return 0;
    return status_call([&] {
        const auto result = static_cast<const IntMapleTree*>(handle)->value.find(key);
        *found = result.has_value();
        if (result) *value = *result;
    });
}
int coolbox_ds_maple_remove(void* handle, int64_t key, int* removed) {
    if (!handle || !removed) return 0;
    return status_call([&] { *removed = static_cast<IntMapleTree*>(handle)->value.remove(key); });
}

void* coolbox_ds_cover_tree_new(int faster, size_t dimensions) {
    if (dimensions == 0) return nullptr;
    try { return new MetricTree(faster != 0, dimensions); } catch (...) { return nullptr; }
}
void coolbox_ds_cover_tree_free(void* handle) { delete static_cast<MetricTree*>(handle); }
int coolbox_ds_cover_tree_build(void* handle, const double* values, size_t count, size_t dimensions) {
    if (!handle || (count > 0 && !values) ||
        dimensions != static_cast<MetricTree*>(handle)->dimensions) return 0;
    return status_call([&] {
        std::vector<DSMetricItem> items(count, DSMetricItem(dimensions));
        for (size_t row = 0; row < count; ++row) {
            std::copy(values + row * dimensions, values + (row + 1) * dimensions, items[row].begin());
        }
        std::visit([&](auto& tree) { tree.build(items); }, static_cast<MetricTree*>(handle)->value);
    });
}
int coolbox_ds_cover_tree_insert(void* handle, const double* item, size_t dimensions) {
    if (!handle || !item || dimensions != static_cast<MetricTree*>(handle)->dimensions) return 0;
    return status_call([&] {
        DSMetricItem value(item, item + dimensions);
        std::visit([&](auto& tree) { tree.insert(value); }, static_cast<MetricTree*>(handle)->value);
    });
}
int coolbox_ds_cover_tree_clear(void* handle) {
    return handle ? status_call([&] {
        std::visit([](auto& tree) { tree.clear(); }, static_cast<MetricTree*>(handle)->value);
    }) : 0;
}
size_t coolbox_ds_cover_tree_size(const void* handle) {
    return handle ? std::visit([](const auto& tree) { return tree.size(); },
                               static_cast<const MetricTree*>(handle)->value) : 0;
}
int coolbox_ds_cover_tree_validate(const void* handle) {
    if (!handle) return 0;
    return std::visit([](const auto& tree) {
        if constexpr (std::is_same_v<std::decay_t<decltype(tree)>, DSCoverTree>) {
            return tree.validate_invariants();
        }
        return true;
    }, static_cast<const MetricTree*>(handle)->value);
}
int coolbox_ds_cover_tree_query(const void* handle, const double* query, size_t dimensions,
                                size_t k, double radius, int use_radius, size_t* count,
                                size_t* indices, double* distances, double* items, size_t capacity) {
    if (!handle || !query || !count || dimensions != static_cast<const MetricTree*>(handle)->dimensions ||
        ((indices || distances || items) && !(indices && distances && items))) return 0;
    return status_call([&] {
        const auto& metric_tree = *static_cast<const MetricTree*>(handle);
        const DSMetricItem point(query, query + dimensions);
        const auto results = std::visit([&](const auto& tree) {
            return use_radius ? tree.radius_search(point, radius) : tree.k_nearest(point, k);
        }, metric_tree.value);
        *count = results.size();
        if (indices && distances && items) {
            const size_t copied = std::min(results.size(), capacity);
            for (size_t i = 0; i < copied; ++i) {
                indices[i] = results[i].index;
                distances[i] = results[i].distance;
                std::copy(results[i].item.begin(), results[i].item.end(), items + i * dimensions);
            }
        }
    });
}

} // extern "C"
