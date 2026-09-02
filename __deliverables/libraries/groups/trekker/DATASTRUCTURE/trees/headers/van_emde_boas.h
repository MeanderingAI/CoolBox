#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace data_structures {

/**
 * Van Emde Boas Tree
 *
 * A data structure for integer keys in the universe [0, universe_size).
 * All operations — insert, remove, contains, minimum, maximum,
 * successor, and predecessor — run in O(log log U) time, where U is
 * the universe size (automatically rounded up to the next power of 2).
 *
 * Sentinel value NONE (UINT64_MAX) is returned by minimum(), maximum(),
 * successor(), and predecessor() when no result exists.
 */
class VanEmdeBoasTree {
public:
    static constexpr uint64_t NONE = UINT64_MAX;

    /**
     * Construct a vEB tree for keys in [0, universe_size).
     * universe_size is rounded up to the next power of 2 internally.
     */
    explicit VanEmdeBoasTree(uint64_t universe_size);

    /** Insert x into the tree. No-op if already present or x >= universe_size. */
    void insert(uint64_t x);

    /** Remove x from the tree. No-op if not present. */
    void remove(uint64_t x);

    /** Return true if x is present in the tree. */
    bool contains(uint64_t x) const;

    /** Return the minimum element, or NONE if the tree is empty. */
    uint64_t minimum() const { return min_; }

    /** Return the maximum element, or NONE if the tree is empty. */
    uint64_t maximum() const { return max_; }

    /** Return true if the tree contains no elements. */
    bool empty() const { return min_ == NONE; }

    /**
     * Return the smallest element strictly greater than x,
     * or NONE if no such element exists.
     */
    uint64_t successor(uint64_t x) const;

    /**
     * Return the largest element strictly less than x,
     * or NONE if no such element exists.
     */
    uint64_t predecessor(uint64_t x) const;

    /** The effective (rounded-up) universe size. */
    uint64_t universe_size() const { return u_; }

private:
    uint64_t u_;     // effective universe size (power of 2)
    uint64_t min_;   // NONE when empty
    uint64_t max_;   // NONE when empty

    std::unique_ptr<VanEmdeBoasTree> summary_;
    std::vector<std::unique_ptr<VanEmdeBoasTree>> clusters_;

    // Dimensioning helpers
    uint64_t sqrt_upper() const;   // ceil(√u_)  — number of clusters
    uint64_t sqrt_lower() const;   // floor(√u_) — cluster universe size

    // Key decomposition
    uint64_t high(uint64_t x)             const { return x / sqrt_lower(); }
    uint64_t low(uint64_t x)              const { return x % sqrt_lower(); }
    uint64_t compose(uint64_t h, uint64_t l) const { return h * sqrt_lower() + l; }

    static uint64_t round_up_pow2(uint64_t n);
};

} // namespace data_structures
