#pragma once
#include <vector>
#include <initializer_list>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace mytrix {

class MytrixVector {
public:
    using value_type = double;
    using container_type = std::vector<value_type>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    MytrixVector() = default;
    explicit MytrixVector(size_t n) : data_(n) {}
    MytrixVector(size_t n, value_type val) : data_(n, val) {}
    MytrixVector(std::initializer_list<value_type> il) : data_(il) {}
    MytrixVector(const container_type& v) : data_(v) {}
    MytrixVector(container_type&& v) : data_(std::move(v)) {}
    const value_type& at(size_t i) const { return data_.at(i); }
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    void resize(size_t n) { data_.resize(n); }
    void assign(size_t n, value_type val) { data_.assign(n, val); }
    void push_back(value_type val) { data_.push_back(val); }
    void clear() { data_.clear(); }

    value_type sum() const { return std::accumulate(data_.begin(), data_.end(), 0.0); }
    void normalize() {
        value_type s = sum();
        if (s != 0.0) for (auto& v : data_) v /= s;
    }
    container_type& data() { return data_; }
    const container_type& data() const { return data_; }

private:
    container_type data_;
};

} // namespace mytrix
