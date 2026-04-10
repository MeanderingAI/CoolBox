#include "tyst_framework.hpp"

#include "egh_filter.h"

#include <string>

using namespace data_structures;

TEST(EGHFilterTest, ContainsInsertedIntegers) {
    EGHFilter<int> filter(128, 0.01);
    filter.insert(5);
    filter.insert(15);
    filter.insert(25);

    EXPECT_TRUE(filter.contains(5));
    EXPECT_TRUE(filter.contains(15));
    EXPECT_TRUE(filter.contains(25));
}

TEST(EGHFilterTest, EmptyFilterRejectsMissingItems) {
    EGHFilter<int> filter(64, 0.01);
    EXPECT_FALSE(filter.contains(999));
}

TEST(EGHFilterTest, HandlesStrings) {
    EGHFilter<std::string> filter(64, 0.02);
    filter.insert("gamma");
    filter.insert("delta");

    EXPECT_TRUE(filter.contains("gamma"));
    EXPECT_TRUE(filter.contains("delta"));
}

TEST(EGHFilterTest, ClearResetsLayers) {
    EGHFilter<int> filter(128, 0.01);
    filter.insert(3);
    filter.insert(4);

    filter.clear();

    EXPECT_FALSE(filter.contains(3));
    EXPECT_FALSE(filter.contains(4));
    EXPECT_EQ(filter.insert_count(), 0u);
    EXPECT_EQ(filter.approximate_count(), 0u);
}

TEST(EGHFilterTest, ExposesPrimeBackedLayout) {
    EGHFilter<int> filter(256, 0.005);

    EXPECT_GE(filter.layer_count(), 2u);
    EXPECT_GT(filter.total_bucket_count(), 0u);
    EXPECT_EQ(filter.moduli().size(), filter.layer_count());
}
