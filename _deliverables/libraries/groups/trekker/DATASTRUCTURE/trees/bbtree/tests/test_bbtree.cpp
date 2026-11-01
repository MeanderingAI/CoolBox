#include "bbtree.hpp"
#include "tyst_framework.hpp"

#include <limits>
#include <map>
#include <numeric>
#include <random>

using data_structures::BBTreeMap;

namespace {

template <typename Tree, typename Map>
void expect_matches(const Tree& tree, const Map& reference) {
    ASSERT_TRUE(tree.validate());
    ASSERT_EQ(tree.size(), reference.size());
    auto actual = tree.cbegin();
    for (const auto& expected : reference) {
        ASSERT_TRUE(actual != tree.cend());
        EXPECT_EQ(actual->first, expected.first);
        EXPECT_EQ(actual->second, expected.second);
        ++actual;
    }
    EXPECT_TRUE(actual == tree.cend());
}

template <typename Tree, typename Map, typename Key>
void expect_bounds(const Tree& tree, const Map& reference, const Key& key) {
    const auto lower = tree.lower_bound(key);
    const auto expected_lower = reference.lower_bound(key);
    EXPECT_EQ(lower == tree.end(), expected_lower == reference.end());
    if (lower != tree.end() && expected_lower != reference.end()) {
        EXPECT_EQ(lower->first, expected_lower->first);
    }
    const auto upper = tree.upper_bound(key);
    const auto expected_upper = reference.upper_bound(key);
    EXPECT_EQ(upper == tree.end(), expected_upper == reference.end());
    if (upper != tree.end() && expected_upper != reference.end()) {
        EXPECT_EQ(upper->first, expected_upper->first);
    }
    EXPECT_EQ(tree.contains(key), reference.find(key) != reference.end());
}

struct ThrowingValue {
    static bool throw_on_copy;
    int value = 0;
    ThrowingValue() = default;
    explicit ThrowingValue(int v) : value(v) {}
    ThrowingValue(const ThrowingValue& other) : value(other.value) {
        if (throw_on_copy) {
            throw std::runtime_error("requested copy failure");
        }
    }
    ThrowingValue(ThrowingValue&&) = default;
};

bool ThrowingValue::throw_on_copy = false;

struct ThrowingCompare {
    std::shared_ptr<bool> fail;
    bool operator()(int a, int b) const {
        if (*fail) {
            throw std::runtime_error("requested comparison failure");
        }
        return a < b;
    }
};

} // namespace

TEST(BBTreeMap, EmptyAndBasicMapOperations) {
    BBTreeMap<int, std::string> tree;
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.height(), 0U);
    EXPECT_TRUE(tree.validate());
    EXPECT_TRUE(tree.begin() == tree.end());
    EXPECT_TRUE(tree.lower_bound(1) == tree.end());
    EXPECT_FALSE(tree.erase(1));
    EXPECT_THROW(tree.at(1), std::out_of_range);
    auto inserted = tree.insert(7, "seven");
    EXPECT_TRUE(inserted.second);
    EXPECT_EQ(inserted.first->first, 7);
    EXPECT_EQ(inserted.first->second, "seven");
    auto duplicate = tree.insert(7, "ignored");
    EXPECT_FALSE(duplicate.second);
    EXPECT_EQ(tree.at(7), "seven");
    auto assigned = tree.insert_or_assign(7, "updated");
    EXPECT_FALSE(assigned.second);
    EXPECT_EQ(assigned.first->second, "updated");
    tree.at(7) = "mutable";
    EXPECT_EQ(tree.find(7)->second, "mutable");
    EXPECT_EQ(tree.size(), 1U);
    EXPECT_EQ(tree.height(), 1U);
    EXPECT_TRUE(tree.upper_bound(7) == tree.end());
    EXPECT_TRUE(tree.erase(7));
    EXPECT_TRUE(tree.empty());
    EXPECT_TRUE(tree.validate());
    EXPECT_EQ(tree.height(), 0U);
}

TEST(BBTreeMap, CascadingSplitsMergesAndRangeScans) {
    BBTreeMap<int, int, std::less<int>, 2> tree;
    std::map<int, int> reference;
    for (int key = 0; key < 1000; ++key) {
        auto result = tree.insert(key, key * 3);
        ASSERT_TRUE(result.second);
        EXPECT_EQ(result.first->first, key);
        EXPECT_EQ(result.first->second, key * 3);
        reference.emplace(key, key * 3);
        ASSERT_TRUE(tree.validate());
    }
    EXPECT_GT(tree.height(), 1U);
    expect_matches(tree, reference);
    for (int key = -1; key <= 1000; ++key) {
        expect_bounds(tree, reference, key);
    }
    int next = 100;
    for (auto it = tree.lower_bound(100); it != tree.lower_bound(200); ++it) {
        EXPECT_EQ(it->first, next++);
    }
    EXPECT_EQ(next, 200);
    for (int key = 0; key < 1000; ++key) {
        EXPECT_TRUE(tree.erase(key));
        reference.erase(key);
        expect_matches(tree, reference);
    }
    EXPECT_EQ(tree.height(), 0U);
    tree.insert(1, 2);
    tree.clear();
    EXPECT_TRUE(tree.empty());
    EXPECT_TRUE(tree.validate());
}

TEST(BBTreeMap, RandomOperationsMatchStdMap) {
    BBTreeMap<int, int, std::less<int>, 3> tree;
    std::map<int, int> reference;
    std::mt19937 random(451234);
    for (int iteration = 0; iteration < 12000; ++iteration) {
        const int key = static_cast<int>(random() % 600) - 300;
        const int value = static_cast<int>(random() % 100000);
        switch (random() % 3) {
            case 0: {
                const auto actual = tree.insert(key, value);
                const auto expected = reference.emplace(key, value);
                EXPECT_EQ(actual.second, expected.second);
                EXPECT_EQ(actual.first->second, expected.first->second);
                break;
            }
            case 1: {
                const auto actual = tree.insert_or_assign(key, value);
                const auto expected = reference.insert_or_assign(key, value);
                EXPECT_EQ(actual.second, expected.second);
                EXPECT_EQ(actual.first->second, expected.first->second);
                break;
            }
            default:
                EXPECT_EQ(tree.erase(key), reference.erase(key) != 0);
        }
        expect_matches(tree, reference);
        expect_bounds(tree, reference, key);
        expect_bounds(tree, reference, static_cast<int>(random() % 800) - 400);
    }
}

TEST(BBTreeMap, DefaultNodeSizeAndReverseInsertionDeletion) {
    BBTreeMap<unsigned, unsigned> tree;
    std::map<unsigned, unsigned> reference;
    for (unsigned key = 2000; key > 0; --key) {
        tree.insert_or_assign(key, key);
        reference.emplace(key, key);
    }
    tree.insert(0U, 0U);
    reference.emplace(0U, 0U);
    tree.insert(std::numeric_limits<unsigned>::max(), 123U);
    reference.emplace(std::numeric_limits<unsigned>::max(), 123U);
    expect_matches(tree, reference);
    expect_bounds(tree, reference, std::numeric_limits<unsigned>::max());
    for (unsigned key = 2000; key > 0; --key) {
        tree.erase(key);
        reference.erase(key);
        expect_matches(tree, reference);
    }
    tree.erase(0);
    tree.erase(std::numeric_limits<unsigned>::max());
    EXPECT_TRUE(tree.validate());
    EXPECT_TRUE(tree.empty());
}

TEST(BBTreeMap, StringsWithSharedPrefixesNullBytesAndHintCollisions) {
    BBTreeMap<std::string, int, std::less<std::string>, 2> tree;
    std::map<std::string, int> reference;
    const std::string prefix = "https://example.test/a/very/long/shared/prefix/";
    std::vector<std::string> keys{"", "a", "aa", "b", std::string("a\0", 2),
                                 std::string("a\0b", 3), std::string("\xff", 1)};
    for (int i = 0; i < 400; ++i) {
        keys.push_back(prefix + "abcdefgh-collision-" + std::to_string(i));
    }
    std::mt19937 random(1234);
    std::shuffle(keys.begin(), keys.end(), random);
    for (std::size_t i = 0; i < keys.size(); ++i) {
        tree.insert(keys[i], static_cast<int>(i));
        reference.emplace(keys[i], static_cast<int>(i));
        ASSERT_TRUE(tree.validate());
    }
    expect_matches(tree, reference);
    for (const auto& key : keys) {
        expect_bounds(tree, reference, key);
        expect_bounds(tree, reference, key + "!");
        EXPECT_EQ(tree.at(key), reference.at(key));
    }
    for (const auto& key : {"https://", "https://example.test/a/very/long/shared/prefix",
                            "https://example.test/a/very/long/shared/prefix0", "z"}) {
        expect_bounds(tree, reference, std::string(key));
    }
    for (const auto& key : keys) {
        tree.erase(key);
        reference.erase(key);
        expect_matches(tree, reference);
    }
}

TEST(BBTreeMap, StatefulCustomComparator) {
    struct Compare {
        bool descending = true;
        bool operator()(int a, int b) const { return descending ? a > b : a < b; }
    };
    BBTreeMap<int, int, Compare, 2> tree(Compare{true});
    std::map<int, int, Compare> reference(Compare{true});
    for (int key = -100; key <= 100; ++key) {
        tree.insert(key, key * 2);
        reference.emplace(key, key * 2);
    }
    expect_matches(tree, reference);
    for (int key = -101; key <= 101; ++key) {
        expect_bounds(tree, reference, key);
    }
    for (int key = -100; key <= 100; key += 2) {
        tree.erase(key);
        reference.erase(key);
        expect_matches(tree, reference);
    }
}

TEST(BBTreeMap, ComparatorEquivalenceUsesOneEntry) {
    struct LengthOrder {
        bool operator()(const std::string& a, const std::string& b) const {
            return a.size() < b.size();
        }
    };
    BBTreeMap<std::string, int, LengthOrder, 2> tree;
    for (std::size_t i = 1; i < 100; ++i) {
        tree.insert(std::string(i, 'a'), static_cast<int>(i));
    }
    EXPECT_FALSE(tree.insert("b", 100).second);
    EXPECT_EQ(tree.at("c"), 1);
    EXPECT_FALSE(tree.insert_or_assign("d", 500).second);
    EXPECT_EQ(tree.at("e"), 500);
    EXPECT_EQ(tree.find("e")->first, "a");
    EXPECT_EQ(tree.size(), 99U);
    EXPECT_TRUE(tree.validate());
    EXPECT_TRUE(tree.erase("f"));
    EXPECT_FALSE(tree.contains("a"));
    EXPECT_TRUE(tree.validate());
}

TEST(BBTreeMap, RandomBinaryStringUpdatesMatchStdMap) {
    BBTreeMap<std::string, int, std::less<void>, 3> tree;
    std::map<std::string, int> reference;
    std::mt19937 random(781234);
    std::vector<std::string> keys{""};
    for (int i = 0; i < 250; ++i) {
        std::string key = i % 2 ? "long/shared/prefix/" : "";
        const auto length = random() % 20;
        for (std::size_t j = 0; j < length; ++j) {
            key.push_back(static_cast<char>(random() % 256));
        }
        keys.push_back(std::move(key));
    }
    for (int i = 0; i < 2500; ++i) {
        const auto& key = keys[random() % keys.size()];
        if (random() % 3 == 0) {
            EXPECT_EQ(tree.erase(key), reference.erase(key) != 0);
        } else {
            tree.insert_or_assign(key, i);
            reference.insert_or_assign(key, i);
        }
        expect_matches(tree, reference);
        expect_bounds(tree, reference, keys[random() % keys.size()]);
    }
}

TEST(BBTreeMap, CopyMoveSwapAndConstIteration) {
    BBTreeMap<int, int, std::less<int>, 2> tree;
    for (int i = 0; i < 100; ++i) {
        tree.insert(i, i);
    }
    auto copy = tree;
    copy.at(0) = 123;
    copy.erase(1);
    EXPECT_EQ(tree.at(0), 0);
    EXPECT_TRUE(tree.contains(1));
    EXPECT_TRUE(copy.validate());
    BBTreeMap<int, int, std::less<int>, 2> moved(std::move(copy));
    EXPECT_TRUE(moved.validate());
    EXPECT_TRUE(copy.empty());
    EXPECT_TRUE(copy.validate());
    copy.insert(400, 400);
    copy = std::move(moved);
    EXPECT_TRUE(copy.validate());
    EXPECT_TRUE(moved.empty());
    moved = tree;
    moved.clear();
    EXPECT_EQ(tree.size(), 100U);
    copy.swap(moved);
    EXPECT_TRUE(copy.empty());
    EXPECT_EQ(moved.at(0), 123);
    BBTreeMap<int, int, std::less<int>, 2>& alias = tree;
    tree = alias;
    EXPECT_TRUE(tree.validate());
    BBTreeMap<int, int, std::less<int>, 2>::const_iterator constant = tree.begin();
    EXPECT_TRUE(constant == tree.cbegin());
    EXPECT_TRUE(tree.begin() == constant);
    EXPECT_EQ((constant++)->first, 0);
    EXPECT_EQ(constant->first, 1);
}

TEST(BBTreeMap, MoveOnlyMappedValues) {
    BBTreeMap<int, std::unique_ptr<int>, std::less<int>, 2> tree;
    for (int i = 0; i < 200; ++i) {
        tree.insert(i, std::make_unique<int>(i));
    }
    auto unused = std::make_unique<int>(999);
    EXPECT_FALSE(tree.insert(1, std::move(unused)).second);
    ASSERT_TRUE(unused != nullptr);
    tree.insert_or_assign(1, std::move(unused));
    EXPECT_EQ(*tree.at(1), 999);
    EXPECT_TRUE(tree.validate());
    for (int i = 0; i < 200; ++i) {
        EXPECT_TRUE(tree.erase(i));
        EXPECT_TRUE(tree.validate());
    }
}

TEST(BBTreeMap, FailedConstructionOrComparisonPreservesTree) {
    BBTreeMap<int, ThrowingValue, std::less<int>, 2> tree;
    for (int i = 0; i < 50; ++i) {
        tree.insert(i, ThrowingValue(i));
    }
    const ThrowingValue value(999);
    ThrowingValue::throw_on_copy = true;
    EXPECT_THROW(tree.insert(100, value), std::runtime_error);
    EXPECT_THROW(tree.insert_or_assign(1, value), std::runtime_error);
    ThrowingValue::throw_on_copy = false;
    EXPECT_EQ(tree.size(), 50U);
    EXPECT_EQ(tree.at(1).value, 1);
    EXPECT_TRUE(tree.validate());
    const auto fail = std::make_shared<bool>(false);
    BBTreeMap<int, int, ThrowingCompare, 2> comparisons(ThrowingCompare{fail});
    for (int i = 0; i < 50; ++i) {
        comparisons.insert(i, i);
    }
    *fail = true;
    EXPECT_THROW(comparisons.insert(100, 100), std::runtime_error);
    EXPECT_THROW(comparisons.erase(1), std::runtime_error);
    EXPECT_THROW(comparisons.find(1), std::runtime_error);
    *fail = false;
    EXPECT_EQ(comparisons.size(), 50U);
    EXPECT_TRUE(comparisons.validate());
}
