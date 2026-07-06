#include "tyst_framework.hpp"

#include "bloom_filter.h"

#include <string>

using namespace data_structures;

TEST(BloomFilterTest, ContainsInsertedIntegers) {
    BloomFilter<int> filter(128, 0.01);
    filter.insert(10);
    filter.insert(42);
    filter.insert(77);

    EXPECT_TRUE(filter.contains(10));
    EXPECT_TRUE(filter.contains(42));
    EXPECT_TRUE(filter.contains(77));
}

TEST(BloomFilterTest, HandlesStrings) {
    BloomFilter<std::string> filter(64, 0.02);
    filter.insert("alpha");
    filter.insert("beta");

    EXPECT_TRUE(filter.contains("alpha"));
    EXPECT_TRUE(filter.contains("beta"));
}

TEST(BloomFilterTest, ClearResetsFilter) {
    BloomFilter<int> filter(64, 0.01);
    filter.insert(1);
    filter.insert(2);

    filter.clear();

    EXPECT_FALSE(filter.contains(1));
    EXPECT_FALSE(filter.contains(2));
    EXPECT_EQ(filter.insert_count(), 0u);
    EXPECT_EQ(filter.approximate_count(), 0u);
}

TEST(BloomFilterTest, TracksApproximateCount) {
    BloomFilter<int> filter(256, 0.01);
    for (int i = 0; i < 100; ++i) {
        filter.insert(i);
    }

    EXPECT_GE(filter.approximate_count(), 50u);
    EXPECT_LE(filter.approximate_count(), 150u);
    EXPECT_GT(filter.set_bit_count(), 0u);
}
