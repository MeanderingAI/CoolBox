#ifndef DATA_STRUCTURES_PROBABILISTIC_FILTER_UTILS_H
#define DATA_STRUCTURES_PROBABILISTIC_FILTER_UTILS_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "../../../MISC/hash/headers/murmur_hash.hpp"

namespace data_structures {
namespace detail {

inline std::uint64_t mix64(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

inline std::size_t sanitize_expected_insertions(std::size_t expected_insertions) {
    return expected_insertions == 0 ? 1 : expected_insertions;
}

inline double sanitize_false_positive_rate(double false_positive_rate) {
    if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0) {
        return 0.01;
    }
    return false_positive_rate;
}

inline bool is_prime(std::size_t value) {
    if (value < 2) {
        return false;
    }
    if (value == 2 || value == 3) {
        return true;
    }
    if (value % 2 == 0 || value % 3 == 0) {
        return false;
    }

    for (std::size_t i = 5; i * i <= value; i += 6) {
        if (value % i == 0 || value % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

inline std::size_t next_prime(std::size_t start) {
    if (start <= 2) {
        return 2;
    }

    if (start % 2 == 0) {
        ++start;
    }

    while (!is_prime(start)) {
        start += 2;
    }

    return start;
}

template<typename T>
inline std::uint64_t murmur_primary_hash(const T& value) {
    return utils::hash::murmur::hash_value(value, 0x9747b28cULL);
}

template<typename T>
inline std::uint64_t murmur_secondary_hash(const T& value) {
    const auto primary = murmur_primary_hash(value);
    const auto secondary = utils::hash::murmur::hash_value(primary, 0xc58f1a7bULL);
    return secondary == 0 ? 0x9e3779b97f4a7c15ULL : secondary;
}

} // namespace detail
} // namespace data_structures

#endif // DATA_STRUCTURES_PROBABILISTIC_FILTER_UTILS_H
