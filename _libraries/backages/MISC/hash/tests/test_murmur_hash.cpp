#include "tyst_framework.hpp"

#include "murmur_hash.hpp"

TEST(MurmurHashTest, ProducesDeterministicHashes) {
    const auto first = utils::hash::murmur::hash_string("coolbox");
    const auto second = utils::hash::murmur::hash_string("coolbox");
    EXPECT_EQ(first, second);
}

TEST(MurmurHashTest, DistinguishesDifferentInputs) {
    const auto first = utils::hash::murmur::hash_string("alpha");
    const auto second = utils::hash::murmur::hash_string("beta");
    EXPECT_NE(first, second);
}

TEST(MurmurHashTest, SeedAffectsResult) {
    const int value = 42;
    const auto first = utils::hash::murmur::hash_value(value, 1);
    const auto second = utils::hash::murmur::hash_value(value, 2);
    EXPECT_NE(first, second);
}
