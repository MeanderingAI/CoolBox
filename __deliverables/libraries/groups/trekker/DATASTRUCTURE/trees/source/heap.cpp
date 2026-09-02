#include "../headers/heap.h"

#include <string>
#include <utility>

namespace data_structures {

template<typename T, typename Compare>
void Heap<T, Compare>::push(const T& value) {
    data_.push_back(value);
    sift_up(data_.size() - 1);
}

template<typename T, typename Compare>
bool Heap<T, Compare>::pop() {
    if (data_.empty()) {
        return false;
    }

    if (data_.size() == 1) {
        data_.pop_back();
        return true;
    }

    std::swap(data_.front(), data_.back());
    data_.pop_back();
    sift_down(0);
    return true;
}

template<typename T, typename Compare>
const T& Heap<T, Compare>::top() const {
    if (data_.empty()) {
        throw std::runtime_error("Heap is empty");
    }
    return data_.front();
}

template<typename T, typename Compare>
void Heap<T, Compare>::sift_up(size_t index) {
    while (index > 0) {
        const size_t parent = (index - 1) / 2;
        if (!should_swap(data_[parent], data_[index])) {
            break;
        }
        std::swap(data_[parent], data_[index]);
        index = parent;
    }
}

template<typename T, typename Compare>
void Heap<T, Compare>::sift_down(size_t index) {
    while (true) {
        const size_t left = index * 2 + 1;
        const size_t right = index * 2 + 2;
        size_t candidate = index;

        if (left < data_.size() && should_swap(data_[candidate], data_[left])) {
            candidate = left;
        }
        if (right < data_.size() && should_swap(data_[candidate], data_[right])) {
            candidate = right;
        }
        if (candidate == index) {
            break;
        }
        std::swap(data_[index], data_[candidate]);
        index = candidate;
    }
}

template<typename T, typename Compare>
bool Heap<T, Compare>::should_swap(const T& parent, const T& child) const {
    return compare_(parent, child);
}

template class Heap<int>;
template class Heap<double>;
template class Heap<std::string>;

} // namespace data_structures
