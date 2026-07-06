#include "tyst_framework.hpp"

#include "cover_tree.h"

#include <cmath>
#include <algorithm>
#include <vector>

using namespace data_structures;

namespace {

struct Point2D {
    double x;
    double y;
};

double euclidean_distance(const Point2D& lhs, const Point2D& rhs) {
    const double dx = lhs.x - rhs.x;
    const double dy = lhs.y - rhs.y;
    return std::sqrt(dx * dx + dy * dy);
}

std::vector<Point2D> sample_points() {
    return {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {5.0, 5.0}};
}

std::vector<MetricTreeQueryResult<Point2D>> brute_force_results(
    const std::vector<Point2D>& points,
    const Point2D& query,
    std::size_t k)
{
    std::vector<MetricTreeQueryResult<Point2D>> results;
    results.reserve(points.size());
    for (std::size_t index = 0; index < points.size(); ++index) {
        results.push_back({index, points[index], euclidean_distance(query, points[index])});
    }

    std::stable_sort(results.begin(), results.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.distance == rhs.distance) {
            return lhs.index < rhs.index;
        }
        return lhs.distance < rhs.distance;
    });

    if (k < results.size()) {
        results.resize(k);
    }
    return results;
}

} // namespace

TEST(CoverTreeTest, BuildAndKNearest) {
    CoverTree<Point2D> tree(euclidean_distance);
    tree.build(sample_points());

    EXPECT_TRUE(tree.validate_invariants());

    const auto results = tree.k_nearest(Point2D{0.1, 0.1}, 3);

    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].index, 0u);
    EXPECT_LE(results[0].distance, results[1].distance);
    EXPECT_LE(results[1].distance, results[2].distance);
}

TEST(CoverTreeTest, InsertAndRadiusSearch) {
    CoverTree<Point2D> tree(euclidean_distance);
    tree.insert({0.0, 0.0});
    tree.insert({0.5, 0.5});
    tree.insert({3.0, 3.0});

    EXPECT_TRUE(tree.validate_invariants());

    const auto results = tree.radius_search(Point2D{0.0, 0.0}, 1.0);

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].index, 0u);
    EXPECT_EQ(results[1].index, 1u);
}

TEST(CoverTreeTest, ClearResetsTree) {
    CoverTree<Point2D> tree(euclidean_distance);
    tree.build(sample_points());
    tree.clear();

    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0u);
    EXPECT_TRUE(tree.k_nearest(Point2D{0.0, 0.0}, 2).empty());
}

TEST(CoverTreeTest, MatchesBruteForceOrdering) {
    const auto points = sample_points();
    CoverTree<Point2D> tree(euclidean_distance);
    tree.build(points);

    EXPECT_TRUE(tree.validate_invariants());

    const auto actual = tree.k_nearest(Point2D{0.1, 0.1}, 4);
    const auto expected = brute_force_results(points, Point2D{0.1, 0.1}, 4);

    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t index = 0; index < actual.size(); ++index) {
        EXPECT_EQ(actual[index].index, expected[index].index);
        EXPECT_DOUBLE_EQ(actual[index].distance, expected[index].distance);
    }
}