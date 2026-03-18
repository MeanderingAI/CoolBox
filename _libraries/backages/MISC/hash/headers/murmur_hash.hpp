#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

namespace utils {
namespace hash {
namespace murmur {

inline std::uint64_t rotl64(std::uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

inline std::uint64_t fmix64(std::uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;
    return k;
}

inline std::uint64_t hash_bytes(const void* data, std::size_t length, std::uint64_t seed = 0xc70f6907UL) {
    constexpr std::uint64_t c1 = 0x87c37b91114253d5ULL;
    constexpr std::uint64_t c2 = 0x4cf5ad432745937fULL;

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    const int nblocks = static_cast<int>(length / 16);

    std::uint64_t h1 = seed;
    std::uint64_t h2 = seed;

    for (int i = 0; i < nblocks; ++i) {
        std::uint64_t k1;
        std::uint64_t k2;
        std::memcpy(&k1, bytes + i * 16, sizeof(std::uint64_t));
        std::memcpy(&k2, bytes + i * 16 + 8, sizeof(std::uint64_t));

        k1 *= c1;
        k1 = rotl64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1 = rotl64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2 = rotl64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    const auto* tail = bytes + nblocks * 16;
    std::uint64_t k1 = 0;
    std::uint64_t k2 = 0;

    switch (length & 15U) {
        case 15: k2 ^= static_cast<std::uint64_t>(tail[14]) << 48U; [[fallthrough]];
        case 14: k2 ^= static_cast<std::uint64_t>(tail[13]) << 40U; [[fallthrough]];
        case 13: k2 ^= static_cast<std::uint64_t>(tail[12]) << 32U; [[fallthrough]];
        case 12: k2 ^= static_cast<std::uint64_t>(tail[11]) << 24U; [[fallthrough]];
        case 11: k2 ^= static_cast<std::uint64_t>(tail[10]) << 16U; [[fallthrough]];
        case 10: k2 ^= static_cast<std::uint64_t>(tail[9]) << 8U; [[fallthrough]];
        case 9:
            k2 ^= static_cast<std::uint64_t>(tail[8]);
            k2 *= c2;
            k2 = rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;
            [[fallthrough]];
        case 8: k1 ^= static_cast<std::uint64_t>(tail[7]) << 56U; [[fallthrough]];
        case 7: k1 ^= static_cast<std::uint64_t>(tail[6]) << 48U; [[fallthrough]];
        case 6: k1 ^= static_cast<std::uint64_t>(tail[5]) << 40U; [[fallthrough]];
        case 5: k1 ^= static_cast<std::uint64_t>(tail[4]) << 32U; [[fallthrough]];
        case 4: k1 ^= static_cast<std::uint64_t>(tail[3]) << 24U; [[fallthrough]];
        case 3: k1 ^= static_cast<std::uint64_t>(tail[2]) << 16U; [[fallthrough]];
        case 2: k1 ^= static_cast<std::uint64_t>(tail[1]) << 8U; [[fallthrough]];
        case 1:
            k1 ^= static_cast<std::uint64_t>(tail[0]);
            k1 *= c1;
            k1 = rotl64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
            [[fallthrough]];
        default:
            break;
    }

    h1 ^= length;
    h2 ^= length;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    return h1;
}

inline std::uint64_t hash_string(std::string_view value, std::uint64_t seed = 0xc70f6907UL) {
    return hash_bytes(value.data(), value.size(), seed);
}

template<typename T>
inline std::uint64_t hash_value(const T& value, std::uint64_t seed = 0xc70f6907UL) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return hash_string(value, seed);
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
        return hash_string(value, seed);
    } else if constexpr (std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*>) {
        return value ? hash_string(value, seed) : hash_string({}, seed);
    } else if constexpr (std::is_trivially_copyable_v<T>) {
        return hash_bytes(&value, sizeof(T), seed);
    } else {
        const auto fallback = std::hash<T>{}(value);
        return hash_bytes(&fallback, sizeof(fallback), seed);
    }
}

} // namespace murmur
} // namespace hash
} // namespace utils
