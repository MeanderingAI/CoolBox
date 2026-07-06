#include <tyst_framework.hpp>
#include "sorts.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace trekker::algorithm::sorts;

// ─── helpers ────────────────────────────────────────────────────────────────

static std::vector<int> make_random(int n, int seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(-1000, 1000);
    std::vector<int> v(n);
    for (auto& x : v) x = dist(rng);
    return v;
}

static std::vector<int> make_sorted(int n) {
    std::vector<int> v(n);
    for (int i = 0; i < n; ++i) v[i] = i;
    return v;
}

static std::vector<int> make_reversed(int n) {
    std::vector<int> v(n);
    for (int i = 0; i < n; ++i) v[i] = n - 1 - i;
    return v;
}

static std::vector<int> make_duplicates(int n) {
    std::vector<int> v(n);
    for (int i = 0; i < n; ++i) v[i] = i % 7;
    return v;
}

template<typename SortFn>
static void check_sort(SortFn fn, const std::vector<int>& input) {
    auto v = input;
    auto expected = v;
    std::sort(expected.begin(), expected.end());
    fn(v.begin(), v.end());
    EXPECT_EQ(v, expected);
}

template<typename SortFn>
static void check_sort_desc(SortFn fn, const std::vector<int>& input) {
    auto v = input;
    auto expected = v;
    std::sort(expected.begin(), expected.end(), std::greater<int>{});
    fn(v.begin(), v.end(), std::greater<int>{});
    EXPECT_EQ(v, expected);
}

// ═══════════════════════════════════════════════════════════════════════════
// Bubble Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(BubbleSortTest, RandomInts) {
    check_sort([](auto b, auto e){ bubble_sort(b, e); }, make_random(200));
}

TEST(BubbleSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ bubble_sort(b, e); }, make_sorted(50));
}

TEST(BubbleSortTest, Reversed) {
    check_sort([](auto b, auto e){ bubble_sort(b, e); }, make_reversed(50));
}

TEST(BubbleSortTest, Duplicates) {
    check_sort([](auto b, auto e){ bubble_sort(b, e); }, make_duplicates(100));
}

TEST(BubbleSortTest, SingleElement) {
    std::vector<int> v{42};
    bubble_sort(v.begin(), v.end());
    EXPECT_EQ(v, (std::vector<int>{42}));
}

TEST(BubbleSortTest, EmptyRange) {
    std::vector<int> v;
    bubble_sort(v.begin(), v.end()); // must not crash
    EXPECT_TRUE(v.empty());
}

TEST(BubbleSortTest, DescendingComparator) {
    check_sort_desc([](auto b, auto e, auto c){ bubble_sort(b, e, c); }, make_random(100));
}

TEST(BubbleSortTest, Stable) {
    // Pairs where first is key, second is insertion order; stable sort must
    // preserve insertion order among equal keys.
    std::vector<std::pair<int,int>> v = {{2,0},{1,1},{2,2},{1,3},{3,4}};
    bubble_sort(v.begin(), v.end(),
                [](auto& a, auto& b){ return a.first < b.first; });
    EXPECT_EQ(v[0], (std::pair<int,int>{1,1}));
    EXPECT_EQ(v[1], (std::pair<int,int>{1,3}));
    EXPECT_EQ(v[2], (std::pair<int,int>{2,0}));
    EXPECT_EQ(v[3], (std::pair<int,int>{2,2}));
}

// ═══════════════════════════════════════════════════════════════════════════
// Insertion Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(InsertionSortTest, RandomInts) {
    check_sort([](auto b, auto e){ insertion_sort(b, e); }, make_random(200));
}

TEST(InsertionSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ insertion_sort(b, e); }, make_sorted(50));
}

TEST(InsertionSortTest, Reversed) {
    check_sort([](auto b, auto e){ insertion_sort(b, e); }, make_reversed(50));
}

TEST(InsertionSortTest, Strings) {
    std::vector<std::string> v = {"banana", "apple", "cherry", "date", "apricot"};
    insertion_sort(v.begin(), v.end());
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(InsertionSortTest, Stable) {
    std::vector<std::pair<int,int>> v = {{2,0},{1,1},{2,2},{1,3}};
    insertion_sort(v.begin(), v.end(),
                   [](auto& a, auto& b){ return a.first < b.first; });
    EXPECT_EQ(v[0].second, 1);
    EXPECT_EQ(v[1].second, 3);
    EXPECT_EQ(v[2].second, 0);
    EXPECT_EQ(v[3].second, 2);
}

// ═══════════════════════════════════════════════════════════════════════════
// Selection Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(SelectionSortTest, RandomInts) {
    check_sort([](auto b, auto e){ selection_sort(b, e); }, make_random(200));
}

TEST(SelectionSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ selection_sort(b, e); }, make_sorted(50));
}

TEST(SelectionSortTest, Reversed) {
    check_sort([](auto b, auto e){ selection_sort(b, e); }, make_reversed(50));
}

TEST(SelectionSortTest, AllSame) {
    std::vector<int> v(20, 5);
    selection_sort(v.begin(), v.end());
    EXPECT_EQ(v, std::vector<int>(20, 5));
}

// ═══════════════════════════════════════════════════════════════════════════
// Shell Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(ShellSortTest, RandomInts) {
    check_sort([](auto b, auto e){ shell_sort(b, e); }, make_random(500));
}

TEST(ShellSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ shell_sort(b, e); }, make_sorted(200));
}

TEST(ShellSortTest, Reversed) {
    check_sort([](auto b, auto e){ shell_sort(b, e); }, make_reversed(200));
}

TEST(ShellSortTest, LargeRandom) {
    check_sort([](auto b, auto e){ shell_sort(b, e); }, make_random(10000, 99));
}

// ═══════════════════════════════════════════════════════════════════════════
// Merge Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(MergeSortTest, RandomInts) {
    check_sort([](auto b, auto e){ merge_sort(b, e); }, make_random(500));
}

TEST(MergeSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ merge_sort(b, e); }, make_sorted(200));
}

TEST(MergeSortTest, Reversed) {
    check_sort([](auto b, auto e){ merge_sort(b, e); }, make_reversed(200));
}

TEST(MergeSortTest, Duplicates) {
    check_sort([](auto b, auto e){ merge_sort(b, e); }, make_duplicates(300));
}

TEST(MergeSortTest, LargeRandom) {
    check_sort([](auto b, auto e){ merge_sort(b, e); }, make_random(50000, 7));
}

TEST(MergeSortTest, Stable) {
    std::vector<std::pair<int,int>> v;
    for (int i = 0; i < 100; ++i) v.push_back({i % 10, i});
    merge_sort(v.begin(), v.end(),
               [](auto& a, auto& b){ return a.first < b.first; });
    // Within each key group, insertion order must be preserved
    for (int key = 0; key < 10; ++key) {
        int prev = -1;
        for (auto& p : v) {
            if (p.first == key) {
                EXPECT_GT(p.second, prev);
                prev = p.second;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Quick Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(QuickSortTest, RandomInts) {
    check_sort([](auto b, auto e){ quick_sort(b, e); }, make_random(500));
}

TEST(QuickSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ quick_sort(b, e); }, make_sorted(200));
}

TEST(QuickSortTest, Reversed) {
    check_sort([](auto b, auto e){ quick_sort(b, e); }, make_reversed(200));
}

TEST(QuickSortTest, Duplicates) {
    check_sort([](auto b, auto e){ quick_sort(b, e); }, make_duplicates(300));
}

TEST(QuickSortTest, LargeRandom) {
    check_sort([](auto b, auto e){ quick_sort(b, e); }, make_random(50000, 13));
}

TEST(QuickSortTest, DescendingComparator) {
    check_sort_desc([](auto b, auto e, auto c){ quick_sort(b, e, c); }, make_random(200));
}

// ═══════════════════════════════════════════════════════════════════════════
// Heap Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(HeapSortTest, RandomInts) {
    check_sort([](auto b, auto e){ heap_sort(b, e); }, make_random(500));
}

TEST(HeapSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ heap_sort(b, e); }, make_sorted(200));
}

TEST(HeapSortTest, Reversed) {
    check_sort([](auto b, auto e){ heap_sort(b, e); }, make_reversed(200));
}

TEST(HeapSortTest, LargeRandom) {
    check_sort([](auto b, auto e){ heap_sort(b, e); }, make_random(50000, 17));
}

TEST(HeapSortTest, AllSame) {
    std::vector<int> v(100, 3);
    heap_sort(v.begin(), v.end());
    EXPECT_EQ(v, std::vector<int>(100, 3));
}

// ═══════════════════════════════════════════════════════════════════════════
// Intro Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(IntroSortTest, RandomInts) {
    check_sort([](auto b, auto e){ intro_sort(b, e, std::less<>{}); }, make_random(500));
}

TEST(IntroSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ intro_sort(b, e, std::less<>{}); }, make_sorted(200));
}

TEST(IntroSortTest, Reversed) {
    check_sort([](auto b, auto e){ intro_sort(b, e, std::less<>{}); }, make_reversed(200));
}

TEST(IntroSortTest, Duplicates) {
    check_sort([](auto b, auto e){ intro_sort(b, e, std::less<>{}); }, make_duplicates(500));
}

TEST(IntroSortTest, LargeRandom) {
    check_sort([](auto b, auto e){ intro_sort(b, e, std::less<>{}); }, make_random(100000, 31));
}

// ═══════════════════════════════════════════════════════════════════════════
// Tim Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(TimSortTest, RandomInts) {
    check_sort([](auto b, auto e){ tim_sort(b, e); }, make_random(500));
}

TEST(TimSortTest, AlreadySorted) {
    check_sort([](auto b, auto e){ tim_sort(b, e); }, make_sorted(200));
}

TEST(TimSortTest, Reversed) {
    check_sort([](auto b, auto e){ tim_sort(b, e); }, make_reversed(200));
}

TEST(TimSortTest, Duplicates) {
    check_sort([](auto b, auto e){ tim_sort(b, e); }, make_duplicates(300));
}

TEST(TimSortTest, LargeRandom) {
    check_sort([](auto b, auto e){ tim_sort(b, e); }, make_random(50000, 23));
}

TEST(TimSortTest, Stable) {
    std::vector<std::pair<int,int>> v;
    for (int i = 0; i < 200; ++i) v.push_back({i % 10, i});
    tim_sort(v.begin(), v.end(),
             [](auto& a, auto& b){ return a.first < b.first; });
    for (int key = 0; key < 10; ++key) {
        int prev = -1;
        for (auto& p : v) {
            if (p.first == key) { EXPECT_GT(p.second, prev); prev = p.second; }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Counting Sort
// ═══════════════════════════════════════════════════════════════════════════

TEST(CountingSortTest, UnsignedInts) {
    std::vector<unsigned> v = {5,3,8,1,9,2,7,0,4,6};
    counting_sort(v.begin(), v.end(), [](unsigned x){ return static_cast<std::size_t>(x); }, 9);
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(CountingSortTest, AutoMaxKey) {
    auto v = make_random(200);
    // shift to non-negative for counting sort
    std::vector<unsigned> u(v.size());
    for (std::size_t i = 0; i < v.size(); ++i) u[i] = static_cast<unsigned>(v[i] + 1000);
    counting_sort(u.begin(), u.end(), [](unsigned x){ return static_cast<std::size_t>(x); });
    EXPECT_TRUE(std::is_sorted(u.begin(), u.end()));
}

TEST(CountingSortTest, Stable) {
    std::vector<std::pair<unsigned,int>> v = {{2u,0},{1u,1},{2u,2},{1u,3},{3u,4}};
    counting_sort(v.begin(), v.end(),
                  [](auto& p) -> std::size_t { return p.first; }, 3);
    EXPECT_EQ(v[0], (std::pair<unsigned,int>{1u,1}));
    EXPECT_EQ(v[1], (std::pair<unsigned,int>{1u,3}));
    EXPECT_EQ(v[2], (std::pair<unsigned,int>{2u,0}));
    EXPECT_EQ(v[3], (std::pair<unsigned,int>{2u,2}));
}

TEST(CountingSortTest, AllSame) {
    std::vector<unsigned> v(50, 7u);
    counting_sort(v.begin(), v.end(), [](unsigned x){ return (std::size_t)x; }, 7);
    EXPECT_EQ(v, std::vector<unsigned>(50, 7u));
}

// ═══════════════════════════════════════════════════════════════════════════
// Radix Sort LSD
// ═══════════════════════════════════════════════════════════════════════════

TEST(RadixSortLSDTest, Uint8) {
    std::vector<uint8_t> v = {255,0,128,64,3,200,17,99};
    radix_sort_lsd(v.begin(), v.end());
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(RadixSortLSDTest, Uint32) {
    std::vector<uint32_t> v = {0xDEAD'BEEF, 0, 1, 0xFF'FF'FF'FF, 42, 1000000};
    radix_sort_lsd(v.begin(), v.end());
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(RadixSortLSDTest, Uint64Large) {
    std::mt19937_64 rng(11);
    std::vector<uint64_t> v(10000);
    for (auto& x : v) x = rng();
    radix_sort_lsd(v.begin(), v.end());
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(RadixSortLSDTest, CustomByteExtractor) {
    // Sort pairs of uint8 by their second byte (byte index 1 of a 2-byte key)
    struct Item { uint16_t key; int order; };
    std::vector<Item> v = {{0x0200,0},{0x0100,1},{0x0300,2},{0x0101,3}};
    radix_sort_lsd(v.begin(), v.end(),
                   [](const Item& i, int b) -> uint8_t {
                       return static_cast<uint8_t>(i.key >> (b * 8));
                   }, 2);
    for (std::size_t i = 1; i < v.size(); ++i)
        EXPECT_LE(v[i-1].key, v[i].key);
}

TEST(RadixSortLSDTest, AllSame) {
    std::vector<uint32_t> v(100, 12345u);
    radix_sort_lsd(v.begin(), v.end());
    EXPECT_EQ(v, std::vector<uint32_t>(100, 12345u));
}

// ═══════════════════════════════════════════════════════════════════════════
// Radix Sort MSD
// ═══════════════════════════════════════════════════════════════════════════

TEST(RadixSortMSDTest, Uint8) {
    std::vector<uint8_t> v = {255,0,128,64,3,200,17,99};
    radix_sort_msd(v.begin(), v.end());
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(RadixSortMSDTest, Uint32) {
    std::vector<uint32_t> v = {0xDEAD'BEEF, 0, 1, 0xFF'FF'FF'FFu, 42, 999999};
    radix_sort_msd(v.begin(), v.end());
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(RadixSortMSDTest, Uint64Large) {
    std::mt19937_64 rng(13);
    std::vector<uint64_t> v(10000);
    for (auto& x : v) x = rng();
    radix_sort_msd(v.begin(), v.end());
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

// ═══════════════════════════════════════════════════════════════════════════
// Cross-algorithm correctness — all must agree on the same input
// ═══════════════════════════════════════════════════════════════════════════

TEST(CrossAlgorithmTest, AllAgreeOnRandomInput) {
    const auto input = make_random(1000, 55);
    auto expected = input;
    std::sort(expected.begin(), expected.end());

    auto test = [&](const char*, auto sort_fn) {
        auto v = input;
        sort_fn(v.begin(), v.end());
        EXPECT_EQ(v, expected);
    };

    test("bubble",    [](auto b, auto e){ bubble_sort(b, e); });
    test("insertion", [](auto b, auto e){ insertion_sort(b, e); });
    test("selection", [](auto b, auto e){ selection_sort(b, e); });
    test("shell",     [](auto b, auto e){ shell_sort(b, e); });
    test("merge",     [](auto b, auto e){ merge_sort(b, e); });
    test("quick",     [](auto b, auto e){ quick_sort(b, e); });
    test("heap",      [](auto b, auto e){ heap_sort(b, e); });
    test("intro",     [](auto b, auto e){ intro_sort(b, e, std::less<>{}); });
    test("tim",       [](auto b, auto e){ tim_sort(b, e); });
}
