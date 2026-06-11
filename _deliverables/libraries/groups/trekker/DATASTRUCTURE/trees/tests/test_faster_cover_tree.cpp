#include "tyst_framework.hpp"

#include "cover_tree.h"
#include "faster_cover_tree.h"

#include <cmath>
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
    return {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {5.0, 5.0}, {0.25, 0.2}};
}

} // namespace

TEST(FasterCoverTreeTest, MatchesCoverTreeNearestNeighbours) {
    CoverTree<Point2D> baseline(euclidean_distance);
    FasterCoverTree<Point2D> tree(euclidean_distance);

    const auto points = sample_points();
    baseline.build(points);
    tree.build(points);

    EXPECT_TRUE(baseline.validate_invariants());
    EXPECT_TRUE(tree.validate_invariants());

    const auto baseline_results = baseline.k_nearest(Point2D{0.1, 0.1}, 4);
    const auto tree_results = tree.k_nearest(Point2D{0.1, 0.1}, 4);

    ASSERT_EQ(baseline_results.size(), tree_results.size());
    for (std::size_t index = 0; index < baseline_results.size(); ++index) {
        EXPECT_EQ(baseline_results[index].index, tree_results[index].index);
        EXPECT_DOUBLE_EQ(baseline_results[index].distance, tree_results[index].distance);
    }
}

TEST(FasterCoverTreeTest, HandlesKGreaterThanSize) {
    FasterCoverTree<Point2D> tree(euclidean_distance);
    tree.build(sample_points());

    EXPECT_TRUE(tree.validate_invariants());

    const auto results = tree.k_nearest(Point2D{0.1, 0.1}, 10);
    EXPECT_EQ(results.size(), sample_points().size());
}

TEST(FasterCoverTreeTest, RadiusSearchMatchesCoverTree) {
    CoverTree<Point2D> baseline(euclidean_distance);
    FasterCoverTree<Point2D> tree(euclidean_distance);

    const auto points = sample_points();
    baseline.build(points);
    tree.build(points);

    EXPECT_TRUE(baseline.validate_invariants());
    EXPECT_TRUE(tree.validate_invariants());

    const auto baseline_results = baseline.radius_search(Point2D{0.1, 0.1}, 1.1);
    const auto tree_results = tree.radius_search(Point2D{0.1, 0.1}, 1.1);

    ASSERT_EQ(baseline_results.size(), tree_results.size());
    for (std::size_t index = 0; index < baseline_results.size(); ++index) {
        EXPECT_EQ(baseline_results[index].index, tree_results[index].index);
        EXPECT_DOUBLE_EQ(baseline_results[index].distance, tree_results[index].distance);
    }
}