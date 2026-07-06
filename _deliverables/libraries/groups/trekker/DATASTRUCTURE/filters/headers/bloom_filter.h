#ifndef DATA_STRUCTURES_BLOOM_FILTER_H
#define DATA_STRUCTURES_BLOOM_FILTER_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "probabilistic_filter_utils.h"

namespace data_structures {

template<typename T>
class BloomFilter {
public:
    explicit BloomFilter(std::size_t expected_insertions = 1024, double false_positive_rate = 0.01)
        : expected_insertions_(detail::sanitize_expected_insertions(expected_insertions)),
          false_positive_rate_(detail::sanitize_false_positive_rate(false_positive_rate)),
          bit_count_(compute_bit_count(expected_insertions_, false_positive_rate_)),
          hash_function_count_(compute_hash_function_count(bit_count_, expected_insertions_)),
          bits_(bit_count_, false),
          inserted_items_(0) {
    }

    void insert(const T& value) {
        const std::uint64_t h1 = primary_hash(value);
        const std::uint64_t h2 = secondary_hash(value);

        for (std::size_t i = 0; i < hash_function_count_; ++i) {
            bits_[index_for(h1, h2, i)] = true;
        }

        ++inserted_items_;
    }

    bool contains(const T& value) const {
        const std::uint64_t h1 = primary_hash(value);
        const std::uint64_t h2 = secondary_hash(value);

        for (std::size_t i = 0; i < hash_function_count_; ++i) {
            if (!bits_[index_for(h1, h2, i)]) {
                return false;
            }
        }

        return true;
    }

    void clear() {
        std::fill(bits_.begin(), bits_.end(), false);
        inserted_items_ = 0;
    }

    std::size_t bit_count() const {
        return bit_count_;
    }

    std::size_t hash_function_count() const {
        return hash_function_count_;
    }

    std::size_t insert_count() const {
        return inserted_items_;
    }

    std::size_t set_bit_count() const {
        return static_cast<std::size_t>(std::count(bits_.begin(), bits_.end(), true));
    }

    std::size_t approximate_count() const {
        const double set_bits = static_cast<double>(set_bit_count());
        const double total_bits = static_cast<double>(bit_count_);
        const double hashes = static_cast<double>(hash_function_count_);

        if (set_bits <= 0.0 || total_bits <= 0.0 || hashes <= 0.0 || set_bits >= total_bits) {
            return set_bits >= total_bits ? inserted_items_ : 0;
        }

        const double estimate = -(total_bits / hashes) * std::log(1.0 - (set_bits / total_bits));
        return static_cast<std::size_t>(std::llround(estimate));
    }

    double estimated_false_positive_rate() const {
        const double total_bits = static_cast<double>(bit_count_);
        const double hashes = static_cast<double>(hash_function_count_);
        const double inserted = static_cast<double>(inserted_items_);

        if (total_bits <= 0.0) {
            return 1.0;
        }

        return std::pow(1.0 - std::exp(-(hashes * inserted) / total_bits), hashes);
    }

private:
    std::size_t expected_insertions_;
    double false_positive_rate_;
    std::size_t bit_count_;
    std::size_t hash_function_count_;
    std::vector<bool> bits_;
    std::size_t inserted_items_;

    static std::size_t compute_bit_count(std::size_t expected_insertions, double false_positive_rate) {
        constexpr double ln2 = 0.6931471805599453;
        const double numerator = -static_cast<double>(expected_insertions) * std::log(false_positive_rate);
        const double denominator = ln2 * ln2;
        return std::max<std::size_t>(8, static_cast<std::size_t>(std::ceil(numerator / denominator)));
    }

    static std::size_t compute_hash_function_count(std::size_t bit_count, std::size_t expected_insertions) {
        constexpr double ln2 = 0.6931471805599453;
        const double estimate = (static_cast<double>(bit_count) / static_cast<double>(expected_insertions)) * ln2;
        return std::max<std::size_t>(1, static_cast<std::size_t>(std::llround(estimate)));
    }

    std::uint64_t primary_hash(const T& value) const {
        return detail::murmur_primary_hash(value);
    }

    std::uint64_t secondary_hash(const T& value) const {
        return detail::murmur_secondary_hash(value);
    }

    std::size_t index_for(std::uint64_t h1, std::uint64_t h2, std::size_t iteration) const {
        const auto combined = detail::mix64(h1 + iteration * h2 + iteration * iteration);
        return static_cast<std::size_t>(combined % bit_count_);
    }
};

} // namespace data_structures

#endif // DATA_STRUCTURES_BLOOM_FILTER_H
