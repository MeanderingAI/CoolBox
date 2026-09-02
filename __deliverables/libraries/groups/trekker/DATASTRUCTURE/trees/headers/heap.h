#ifndef DATA_STRUCTURES_HEAP_H
#define DATA_STRUCTURES_HEAP_H

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

namespace data_structures {

template<typename T, typename Compare = std::less<T>>
class Heap {
public:
    Heap() = default;

    explicit Heap(const Compare& compare)
        : data_(), compare_(compare) {
    }

    void push(const T& value);
    bool pop();

    const T& top() const;

    size_t size() const {
        return data_.size();
    }

    bool empty() const {
        return data_.empty();
    }

    void clear() {
        data_.clear();
    }

    std::vector<T> values() const {
        return data_;
    }

private:
    std::vector<T> data_;
    Compare compare_;

    void sift_up(size_t index);
    void sift_down(size_t index);
    bool should_swap(const T& parent, const T& child) const;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_HEAP_H
