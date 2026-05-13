#pragma once

/**
 * trekker::algorithm::sorts
 * ──────────────────────────────────────────────────────────────────────────
 * Header-only collection of sorting algorithms, all operating on random-
 * access ranges specified by iterators (begin / end) with an optional
 * comparator (defaulting to std::less<> — ascending order).
 *
 * Algorithms provided:
 *   Comparison-based
 *     bubble_sort       — O(n²) stable, tiny code, good for nearly-sorted
 *     insertion_sort    — O(n²) stable, excellent on small / nearly-sorted
 *     selection_sort    — O(n²) unstable, minimal swaps
 *     shell_sort        — O(n log² n) ~, unstable, gap-sequence insertion
 *     merge_sort        — O(n log n) stable, O(n) extra memory
 *     quick_sort        — O(n log n) avg, O(n²) worst, unstable, in-place
 *                         (median-of-3 pivot, switches to insertion_sort
 *                          below a threshold)
 *     heap_sort         — O(n log n) guaranteed, unstable, in-place
 *     intro_sort        — O(n log n) worst, unstable, in-place
 *                         (quick_sort + heap_sort + insertion_sort hybrid)
 *     tim_sort          — O(n log n) stable, O(n) extra
 *                         (insertion_sort on runs + merge pass; simplified)
 *
 *   Non-comparison-based (integers / byte keys)
 *     counting_sort     — O(n + k), stable, requires bounded non-negative
 *                         integer keys; key_fn maps each element to size_t
 *     radix_sort_lsd    — O(d·n), stable, base-256 LSD; works on any type
 *                         that byte_fn can decompose into bytes
 *     radix_sort_msd    — O(d·n) avg, base-256 MSD in-place (American flag)
 *
 * All comparison-based sorts satisfy:
 *   sort(first, last)           — ascending using operator<
 *   sort(first, last, comp)     — ordered by comp(a, b)  [comp(a,b)==true → a before b]
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <vector>

namespace trekker {
namespace algorithm {
namespace sorts {

// ═══════════════════════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════════════════════

namespace detail {

// Insertion sort over [first, last) — used as base case inside hybrids.
template<typename It, typename Cmp>
void insertion_sort_impl(It first, It last, Cmp cmp) {
    if (first == last) return;
    for (It i = std::next(first); i != last; ++i) {
        auto key = std::move(*i);
        It j = i;
        while (j != first && cmp(key, *std::prev(j))) {
            *j = std::move(*std::prev(j));
            --j;
        }
        *j = std::move(key);
    }
}

// Median-of-three pivot selection; returns iterator to chosen pivot.
template<typename It, typename Cmp>
It median_of_three(It first, It last, Cmp cmp) {
    It mid = first + std::distance(first, last) / 2;
    It l   = std::prev(last);
    if (cmp(*mid, *first)) std::iter_swap(mid, first);
    if (cmp(*l,   *first)) std::iter_swap(l,   first);
    if (cmp(*mid, *l))     std::iter_swap(mid, l);
    return l; // pivot now at l
}

// Hoare-style partition around pivot at std::prev(last); returns split point.
template<typename It, typename Cmp>
It partition(It first, It last, Cmp cmp) {
    auto pivot = *std::prev(last);
    It i = first;
    It j = std::prev(last) - 1;
    while (true) {
        while (i <= j && cmp(*i, pivot))  ++i;
        while (j >= i && !cmp(*j, pivot)) { if (j == first) break; --j; }
        if (i >= j) break;
        std::iter_swap(i, j);
        ++i; if (j != first) --j;
    }
    std::iter_swap(i, std::prev(last));
    return i;
}

// Merge two sorted halves [first, mid) and [mid, last) into place.
template<typename It, typename Cmp>
void merge_halves(It first, It mid, It last, Cmp cmp) {
    using T = typename std::iterator_traits<It>::value_type;
    std::vector<T> buf(first, last);
    auto a = buf.begin();
    auto b = buf.begin() + std::distance(first, mid);
    auto ae = b, be = buf.end();
    It out = first;
    while (a != ae && b != be)
        *out++ = cmp(*b, *a) ? std::move(*b++) : std::move(*a++);
    std::move(a, ae, out);
    std::move(b, be, out);
}

// Heap helpers (0-based)
template<typename It, typename Cmp>
void sift_down(It first, std::ptrdiff_t n, std::ptrdiff_t root, Cmp cmp) {
    while (true) {
        std::ptrdiff_t largest = root;
        std::ptrdiff_t l = 2 * root + 1;
        std::ptrdiff_t r = 2 * root + 2;
        if (l < n && cmp(*(first + largest), *(first + l))) largest = l;
        if (r < n && cmp(*(first + largest), *(first + r))) largest = r;
        if (largest == root) break;
        std::iter_swap(first + root, first + largest);
        root = largest;
    }
}

template<typename It, typename Cmp>
void make_heap_impl(It first, std::ptrdiff_t n, Cmp cmp) {
    for (std::ptrdiff_t i = n / 2 - 1; i >= 0; --i)
        sift_down(first, n, i, cmp);
}

// Shell sort gap sequences (Ciura 2001)
static constexpr std::ptrdiff_t SHELL_GAPS[] = {
    701, 301, 132, 57, 23, 10, 4, 1
};

} // namespace detail

// ═══════════════════════════════════════════════════════════════════════════
// Threshold below which hybrids switch to insertion_sort
// ═══════════════════════════════════════════════════════════════════════════

static constexpr std::ptrdiff_t INSERTION_THRESHOLD = 16;

// ═══════════════════════════════════════════════════════════════════════════
// 1. Bubble Sort  — O(n²) stable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Stable, in-place.  Terminates early if the range is already sorted.
 * Time: O(n²) worst/avg, O(n) best (early exit).  Space: O(1).
 */
template<typename It, typename Cmp = std::less<>>
void bubble_sort(It first, It last, Cmp cmp = {}) {
    if (first == last) return;
    bool swapped;
    do {
        swapped = false;
        It cur = first, nxt = std::next(first);
        while (nxt != last) {
            if (cmp(*nxt, *cur)) {
                std::iter_swap(cur, nxt);
                swapped = true;
            }
            cur = nxt++;
        }
        --last; // largest element already bubbled to end
    } while (swapped);
}

// ═══════════════════════════════════════════════════════════════════════════
// 2. Insertion Sort  — O(n²) stable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Stable, in-place.  Optimal for small or nearly-sorted ranges.
 * Time: O(n²) worst/avg, O(n) best.  Space: O(1).
 */
template<typename It, typename Cmp = std::less<>>
void insertion_sort(It first, It last, Cmp cmp = {}) {
    detail::insertion_sort_impl(first, last, cmp);
}

// ═══════════════════════════════════════════════════════════════════════════
// 3. Selection Sort  — O(n²) unstable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Unstable, in-place.  Minimises the number of writes (at most n swaps).
 * Time: O(n²) all cases.  Space: O(1).
 */
template<typename It, typename Cmp = std::less<>>
void selection_sort(It first, It last, Cmp cmp = {}) {
    for (It i = first; i != last; ++i) {
        It min_it = std::min_element(i, last, cmp);
        if (min_it != i) std::iter_swap(i, min_it);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 4. Shell Sort  — O(n log² n) unstable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Unstable, in-place.  Uses Ciura (2001) gap sequence.
 * Time: O(n log² n) empirical, O(n^(4/3)) theoretical.  Space: O(1).
 */
template<typename It, typename Cmp = std::less<>>
void shell_sort(It first, It last, Cmp cmp = {}) {
    std::ptrdiff_t n = std::distance(first, last);
    for (std::ptrdiff_t gap : detail::SHELL_GAPS) {
        if (gap >= n) continue;
        for (std::ptrdiff_t i = gap; i < n; ++i) {
            auto tmp = std::move(*(first + i));
            std::ptrdiff_t j = i;
            while (j >= gap && cmp(tmp, *(first + j - gap))) {
                *(first + j) = std::move(*(first + j - gap));
                j -= gap;
            }
            *(first + j) = std::move(tmp);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 5. Merge Sort  — O(n log n) stable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Stable, requires O(n) extra memory.
 * Time: O(n log n) all cases.  Space: O(n).
 */
template<typename It, typename Cmp = std::less<>>
void merge_sort(It first, It last, Cmp cmp = {}) {
    std::ptrdiff_t n = std::distance(first, last);
    if (n <= INSERTION_THRESHOLD) {
        detail::insertion_sort_impl(first, last, cmp);
        return;
    }
    It mid = first + n / 2;
    merge_sort(first, mid, cmp);
    merge_sort(mid,  last, cmp);
    detail::merge_halves(first, mid, last, cmp);
}

// ═══════════════════════════════════════════════════════════════════════════
// 6. Quick Sort  — O(n log n) avg, O(n²) worst; unstable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Unstable, in-place.  Median-of-three pivot; falls back to insertion_sort
 * for small sub-ranges (INSERTION_THRESHOLD).
 * Time: O(n log n) avg, O(n²) worst.  Space: O(log n) stack.
 */
template<typename It, typename Cmp = std::less<>>
void quick_sort(It first, It last, Cmp cmp = {}) {
    while (std::distance(first, last) > INSERTION_THRESHOLD) {
        detail::median_of_three(first, last, cmp);
        It pivot = detail::partition(first, last, cmp);
        // Recurse into smaller half, iterate over larger (reduces stack depth)
        if (std::distance(first, pivot) < std::distance(pivot, last)) {
            quick_sort(first, pivot, cmp);
            first = std::next(pivot);
        } else {
            quick_sort(std::next(pivot), last, cmp);
            last = pivot;
        }
    }
    detail::insertion_sort_impl(first, last, cmp);
}

// ═══════════════════════════════════════════════════════════════════════════
// 7. Heap Sort  — O(n log n) guaranteed; unstable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Unstable, in-place.  No worst-case degradation.
 * Time: O(n log n) all cases.  Space: O(1).
 */
template<typename It, typename Cmp = std::less<>>
void heap_sort(It first, It last, Cmp cmp = {}) {
    std::ptrdiff_t n = std::distance(first, last);
    detail::make_heap_impl(first, n, cmp);
    for (std::ptrdiff_t i = n - 1; i > 0; --i) {
        std::iter_swap(first, first + i);
        detail::sift_down(first, i, 0, cmp);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 8. Intro Sort  — O(n log n) worst; unstable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Unstable, in-place.  Combines quick_sort, heap_sort, and insertion_sort.
 * The depth limit prevents O(n²) worst-case (same strategy as std::sort).
 * Time: O(n log n) all cases.  Space: O(log n) stack.
 */
template<typename It, typename Cmp = std::less<>>
void intro_sort(It first, It last, Cmp cmp = {});

namespace detail {
template<typename It, typename Cmp>
void intro_sort_impl(It first, It last, int depth_limit, Cmp cmp) {
    while (std::distance(first, last) > INSERTION_THRESHOLD) {
        if (depth_limit == 0) {
            // Switch to heap sort to guarantee O(n log n)
            std::ptrdiff_t n = std::distance(first, last);
            make_heap_impl(first, n, cmp);
            for (std::ptrdiff_t i = n - 1; i > 0; --i) {
                std::iter_swap(first, first + i);
                sift_down(first, i, 0, cmp);
            }
            return;
        }
        --depth_limit;
        median_of_three(first, last, cmp);
        It pivot = partition(first, last, cmp);
        if (std::distance(first, pivot) < std::distance(pivot, last)) {
            intro_sort_impl(first, pivot, depth_limit, cmp);
            first = std::next(pivot);
        } else {
            intro_sort_impl(std::next(pivot), last, depth_limit, cmp);
            last = pivot;
        }
    }
    insertion_sort_impl(first, last, cmp);
}
} // namespace detail

template<typename It, typename Cmp>
void intro_sort(It first, It last, Cmp cmp) {
    std::ptrdiff_t n = std::distance(first, last);
    if (n <= 1) return;
    int depth = 0;
    std::ptrdiff_t m = n;
    while (m > 1) { m >>= 1; ++depth; }
    depth *= 2; // 2 × floor(log₂ n)
    detail::intro_sort_impl(first, last, depth, cmp);
}

// Default-comparator overload
template<typename It>
void intro_sort(It first, It last) {
    intro_sort(first, last, std::less<>{});
}

// ═══════════════════════════════════════════════════════════════════════════
// 9. Tim Sort (simplified)  — O(n log n) stable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Stable, O(n) extra memory.  Identifies natural ascending/descending runs
 * (extended to minrun = 32 with insertion sort), then merges using a stack.
 * This is a simplified but correct implementation; it does not implement the
 * full galloping optimisation from Python's timsort.
 * Time: O(n log n) worst, O(n) best (already sorted).  Space: O(n).
 */
template<typename It, typename Cmp = std::less<>>
void tim_sort(It first, It last, Cmp cmp = {}) {
    static constexpr std::ptrdiff_t MIN_RUN = 32;
    std::ptrdiff_t n = std::distance(first, last);
    if (n <= 1) return;

    // Step 1: sort all runs of length MIN_RUN with insertion_sort
    for (std::ptrdiff_t lo = 0; lo < n; lo += MIN_RUN) {
        std::ptrdiff_t hi = std::min(lo + MIN_RUN, n);
        detail::insertion_sort_impl(first + lo, first + hi, cmp);
    }

    // Step 2: iteratively merge runs, doubling the merge size each pass
    for (std::ptrdiff_t size = MIN_RUN; size < n; size *= 2) {
        for (std::ptrdiff_t lo = 0; lo < n; lo += 2 * size) {
            std::ptrdiff_t mid = std::min(lo + size, n);
            std::ptrdiff_t hi  = std::min(lo + 2 * size, n);
            if (mid < hi)
                detail::merge_halves(first + lo, first + mid, first + hi, cmp);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 10. Counting Sort  — O(n + k); stable; integers only
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Stable, requires O(k) extra memory where k = max_key - min_key + 1.
 * Works on any type T via a key_fn: T → size_t.
 * Caller must ensure all keys are in [0, max_key].
 *
 * @param first    Range begin.
 * @param last     Range end.
 * @param key_fn   Maps each element to a non-negative integer key.
 * @param max_key  Inclusive upper bound on key_fn output.  Pass 0 to
 *                 auto-compute (requires a full scan to find the maximum).
 */
template<typename It, typename KeyFn>
void counting_sort(It first, It last, KeyFn key_fn, std::size_t max_key = 0) {
    if (first == last) return;
    using T = typename std::iterator_traits<It>::value_type;

    if (max_key == 0) {
        for (auto it = first; it != last; ++it) {
            std::size_t k = key_fn(*it);
            if (k > max_key) max_key = k;
        }
    }

    std::vector<std::size_t> count(max_key + 1, 0);
    for (auto it = first; it != last; ++it)
        ++count[key_fn(*it)];

    // Prefix sums → starting positions
    for (std::size_t i = 1; i <= max_key; ++i)
        count[i] += count[i - 1];

    // Build output in reverse order to maintain stability
    std::ptrdiff_t n = std::distance(first, last);
    std::vector<T> out(n);
    for (auto it = std::prev(last); ; --it) {
        out[--count[key_fn(*it)]] = std::move(*it);
        if (it == first) break;
    }
    std::move(out.begin(), out.end(), first);
}

// ═══════════════════════════════════════════════════════════════════════════
// 11. Radix Sort LSD (Least-Significant Digit first)  — O(d·n) stable
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Stable, base-256, LSD.  Works on any type T where byte_fn(elem, byte_idx)
 * returns the byte at position byte_idx (0 = least-significant byte) as a
 * uint8_t, and num_bytes is the total number of bytes per element.
 *
 * Convenience overload for built-in unsigned integers provided below.
 *
 * Time: O(num_bytes · n).  Space: O(n).
 */
template<typename It, typename ByteFn>
void radix_sort_lsd(It first, It last, ByteFn byte_fn, int num_bytes) {
    if (first == last || num_bytes == 0) return;
    using T = typename std::iterator_traits<It>::value_type;
    std::ptrdiff_t n = std::distance(first, last);
    std::vector<T> buf(n);

    for (int b = 0; b < num_bytes; ++b) {
        std::size_t count[256] = {};
        for (auto it = first; it != last; ++it)
            ++count[byte_fn(*it, b)];
        std::size_t prefix[256];
        prefix[0] = 0;
        for (int i = 1; i < 256; ++i)
            prefix[i] = prefix[i - 1] + count[i - 1];
        for (auto it = first; it != last; ++it)
            buf[prefix[byte_fn(*it, b)]++] = std::move(*it);
        std::move(buf.begin(), buf.end(), first);
    }
}

/// Convenience overload for unsigned integral types (uint8..uint64).
template<typename It>
void radix_sort_lsd(It first, It last) {
    using T = typename std::iterator_traits<It>::value_type;
    static_assert(std::is_unsigned<T>::value,
                  "radix_sort_lsd convenience overload requires unsigned integer type");
    constexpr int bytes = static_cast<int>(sizeof(T));
    radix_sort_lsd(first, last,
                   [](T v, int b) -> uint8_t {
                       return static_cast<uint8_t>(v >> (b * 8));
                   },
                   bytes);
}

// ═══════════════════════════════════════════════════════════════════════════
// 12. Radix Sort MSD (Most-Significant Digit first)  — O(d·n) avg in-place
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Unstable (in-place American-flag sort), base-256, MSD.
 * Falls back to insertion_sort for sub-ranges of size ≤ INSERTION_THRESHOLD.
 * The same byte_fn / num_bytes convention as radix_sort_lsd applies.
 *
 * Time: O(num_bytes · n) avg.  Space: O(num_bytes) stack + O(n·256) counts.
 */
template<typename It, typename ByteFn>
void radix_sort_msd(It first, It last, ByteFn byte_fn, int num_bytes) {
    if (std::distance(first, last) <= INSERTION_THRESHOLD || num_bytes == 0) {
        detail::insertion_sort_impl(first, last, std::less<>{});
        return;
    }
    using T = typename std::iterator_traits<It>::value_type;
    std::ptrdiff_t n = std::distance(first, last);
    int b = num_bytes - 1; // current byte (most-significant first)

    // Count
    std::size_t count[256] = {};
    for (auto it = first; it != last; ++it)
        ++count[byte_fn(*it, b)];

    // Prefix sums
    std::size_t starts[256], ends[256];
    starts[0] = 0;
    for (int i = 1; i < 256; ++i)
        starts[i] = starts[i - 1] + count[i - 1];
    for (int i = 0; i < 256; ++i)
        ends[i] = starts[i] + count[i];

    // In-place permutation (American-flag)
    {
        std::size_t cur[256];
        for (int i = 0; i < 256; ++i) cur[i] = starts[i];
        for (std::ptrdiff_t i = 0; i < n; ) {
            uint8_t bkt = byte_fn(*(first + i), b);
            if (i < static_cast<std::ptrdiff_t>(cur[bkt]) &&
                static_cast<std::ptrdiff_t>(cur[bkt]) < static_cast<std::ptrdiff_t>(ends[bkt])) {
                ++cur[bkt];
                ++i;
            } else if (static_cast<std::ptrdiff_t>(cur[bkt]) == i) {
                ++cur[bkt];
                ++i;
            } else {
                std::iter_swap(first + i, first + cur[bkt]);
                ++cur[bkt];
            }
        }
    }

    // Recurse into each bucket
    if (num_bytes > 1) {
        for (int i = 0; i < 256; ++i) {
            if (count[i] > 1)
                radix_sort_msd(first + starts[i], first + ends[i],
                               byte_fn, num_bytes - 1);
        }
    }
}

/// Convenience overload for unsigned integral types.
template<typename It>
void radix_sort_msd(It first, It last) {
    using T = typename std::iterator_traits<It>::value_type;
    static_assert(std::is_unsigned<T>::value,
                  "radix_sort_msd convenience overload requires unsigned integer type");
    constexpr int bytes = static_cast<int>(sizeof(T));
    radix_sort_msd(first, last,
                   [](T v, int b) -> uint8_t {
                       return static_cast<uint8_t>(v >> (b * 8));
                   },
                   bytes);
}

} // namespace sorts
} // namespace algorithm
} // namespace trekker
