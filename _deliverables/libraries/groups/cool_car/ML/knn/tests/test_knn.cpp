#include "tyst_framework.hpp"
#include "knn.h"

#include "mytrix_eigen_compat.hpp"
#include <cmath>
#include <random>
#include <vector>

using namespace ml;
using KNNd = KNN<double>;
using MatrixT = KNNd::MatrixT;
using VectorT = KNNd::VectorT;

// ===================================================================
// Helper: simple 2D classification dataset (two classes)
// ===================================================================
static std::pair<MatrixT, VectorT> make_classification_data() {
    // Class 0 centred around (0, 0), Class 1 centred around (5, 5)
    const int n_per_class = 20;
    MatrixT X(2 * n_per_class, 2);
    VectorT y(2 * n_per_class);

    std::mt19937 rng(42);
    std::normal_distribution<double> noise(0, 0.5);

    for (int i = 0; i < n_per_class; ++i) {
        X(i, 0) = 0 + noise(rng);
        X(i, 1) = 0 + noise(rng);
        y(i) = 0;

        X(n_per_class + i, 0) = 5 + noise(rng);
        X(n_per_class + i, 1) = 5 + noise(rng);
        y(n_per_class + i) = 1;
    }
    return {X, y};
}

// ===================================================================
// Helper: simple 1D regression dataset (y = 2x + 1)
// ===================================================================
static std::pair<MatrixT, VectorT> make_regression_data() {
    const int n = 30;
    MatrixT X(n, 1);
    VectorT y(n);

    for (int i = 0; i < n; ++i) {
        double x = static_cast<double>(i) / n * 10.0;
        X(i, 0) = x;
        y(i) = 2.0 * x + 1.0;
    }
    return {X, y};
}

// ===================================================================
// Construction tests
// ===================================================================

TEST(KNNTest, ConstructionDefaults) {
    KNNd knn;
    EXPECT_EQ(knn.k(), 5);
    EXPECT_EQ(knn.metric(), DistanceMetric::EUCLIDEAN);
    EXPECT_EQ(knn.task(), TaskType::CLASSIFICATION);
    EXPECT_EQ(knn.weight(), WeightType::UNIFORM);
    EXPECT_FALSE(knn.is_fitted());
    EXPECT_EQ(knn.n_samples(), 0);
    EXPECT_EQ(knn.n_features(), 0);
}

TEST(KNNTest, ConstructionCustom) {
    KNNd knn(3, DistanceMetric::MANHATTAN, TaskType::REGRESSION, WeightType::DISTANCE);
    EXPECT_EQ(knn.k(), 3);
    EXPECT_EQ(knn.metric(), DistanceMetric::MANHATTAN);
    EXPECT_EQ(knn.task(), TaskType::REGRESSION);
    EXPECT_EQ(knn.weight(), WeightType::DISTANCE);
}

TEST(KNNTest, ConstructionInvalidK) {
    EXPECT_THROW(KNNd(0), std::invalid_argument);
    EXPECT_THROW(KNNd(-1), std::invalid_argument);
}

TEST(KNNTest, ConstructionInvalidMinkowskiP) {
    EXPECT_THROW(KNNd(3, DistanceMetric::MINKOWSKI, TaskType::CLASSIFICATION,
                       WeightType::UNIFORM, 0.5), std::invalid_argument);
}

// ===================================================================
// Fit tests
// ===================================================================

TEST(KNNTest, FitSuccess) {
    auto [X, y] = make_classification_data();
    KNNd knn(3);
    knn.fit(X, y);
    EXPECT_TRUE(knn.is_fitted());
    EXPECT_EQ(knn.n_samples(), 40);
    EXPECT_EQ(knn.n_features(), 2);
}

TEST(KNNTest, FitMismatch) {
    MatrixT X(5, 2);
    VectorT y(3);
    KNNd knn(3);
    EXPECT_THROW(knn.fit(X, y), std::invalid_argument);
}

TEST(KNNTest, FitEmpty) {
    MatrixT X(0, 2);
    VectorT y(0);
    KNNd knn(3);
    EXPECT_THROW(knn.fit(X, y), std::invalid_argument);
}

// ===================================================================
// Predict before fit
// ===================================================================

TEST(KNNTest, PredictBeforeFit) {
    KNNd knn(3);
    MatrixT X(1, 2);
    X << 1, 2;
    EXPECT_THROW(knn.predict(X), std::logic_error);
}

// ===================================================================
// Classification tests
// ===================================================================

TEST(KNNTest, ClassificationSimple) {
    auto [X, y] = make_classification_data();
    KNNd knn(3, DistanceMetric::EUCLIDEAN, TaskType::CLASSIFICATION);
    knn.fit(X, y);

    // Query near class 0
    MatrixT q0(1, 2);
    q0 << 0.1, -0.1;
    EXPECT_DOUBLE_EQ(knn.predict(q0)(0), 0.0);

    // Query near class 1
    MatrixT q1(1, 2);
    q1 << 5.1, 4.9;
    EXPECT_DOUBLE_EQ(knn.predict(q1)(0), 1.0);
}

TEST(KNNTest, ClassificationAccuracy) {
    auto [X, y] = make_classification_data();
    KNNd knn(5, DistanceMetric::EUCLIDEAN, TaskType::CLASSIFICATION);
    knn.fit(X, y);

    // Score on training data should be high for well-separated clusters
    double acc = knn.score(X, y);
    EXPECT_GE(acc, 0.95);
}

TEST(KNNTest, ClassificationDistanceWeighted) {
    auto [X, y] = make_classification_data();
    KNNd knn(5, DistanceMetric::EUCLIDEAN, TaskType::CLASSIFICATION, WeightType::DISTANCE);
    knn.fit(X, y);

    double acc = knn.score(X, y);
    EXPECT_GE(acc, 0.95);
}

// ===================================================================
// Regression tests
// ===================================================================

TEST(KNNTest, RegressionSimple) {
    auto [X, y] = make_regression_data();
    KNNd knn(3, DistanceMetric::EUCLIDEAN, TaskType::REGRESSION);
    knn.fit(X, y);

    // Query at x=5.0 → y should be ~11.0
    MatrixT q(1, 1);
    q << 5.0;
    double pred = knn.predict(q)(0);
    EXPECT_NEAR(pred, 11.0, 1.5);
}

TEST(KNNTest, RegressionR2) {
    auto [X, y] = make_regression_data();
    KNNd knn(3, DistanceMetric::EUCLIDEAN, TaskType::REGRESSION);
    knn.fit(X, y);

    double r2 = knn.score(X, y);
    EXPECT_GE(r2, 0.9);
}

TEST(KNNTest, RegressionDistanceWeighted) {
    auto [X, y] = make_regression_data();
    KNNd knn(3, DistanceMetric::EUCLIDEAN, TaskType::REGRESSION, WeightType::DISTANCE);
    knn.fit(X, y);

    double r2 = knn.score(X, y);
    EXPECT_GE(r2, 0.9);
}

// ===================================================================
// Distance metric tests
// ===================================================================

TEST(KNNTest, ManhattanDistance) {
    auto [X, y] = make_classification_data();
    KNNd knn(3, DistanceMetric::MANHATTAN, TaskType::CLASSIFICATION);
    knn.fit(X, y);

    double acc = knn.score(X, y);
    EXPECT_GE(acc, 0.95);
}

TEST(KNNTest, MinkowskiDistance) {
    auto [X, y] = make_classification_data();
    KNNd knn(3, DistanceMetric::MINKOWSKI, TaskType::CLASSIFICATION, WeightType::UNIFORM, 3.0);
    knn.fit(X, y);

    double acc = knn.score(X, y);
    EXPECT_GE(acc, 0.95);
}

TEST(KNNTest, ChebyshevDistance) {
    auto [X, y] = make_classification_data();
    KNNd knn(3, DistanceMetric::CHEBYSHEV, TaskType::CLASSIFICATION);
    knn.fit(X, y);

    double acc = knn.score(X, y);
    EXPECT_GE(acc, 0.95);
}

// ===================================================================
// kneighbours test
// ===================================================================

TEST(KNNTest, KNeighbours) {
    MatrixT X(4, 2);
    X << 0, 0,
         1, 0,
         0, 1,
         10, 10;
    VectorT y(4);
    y << 0, 0, 0, 1;

    KNNd knn(3);
    knn.fit(X, y);

    VectorT query(2);
    query << 0.1, 0.1;
    auto neighbours = knn.kneighbours(query);

    EXPECT_EQ(static_cast<int>(neighbours.size()), 3);
    // Closest should be index 0 (at origin)
    EXPECT_EQ(neighbours[0].index, 0);
    // Sorted ascending by distance
    for (size_t i = 1; i < neighbours.size(); ++i) {
        EXPECT_GE(neighbours[i].distance, neighbours[i - 1].distance);
    }
}

// ===================================================================
// Predict with k > n_samples
// ===================================================================

TEST(KNNTest, KLargerThanNSamples) {
    MatrixT X(3, 2);
    X << 0, 0,  1, 0,  0, 1;
    VectorT y(3);
    y << 0, 1, 0;

    KNNd knn(10);  // k=10 but only 3 samples
    knn.fit(X, y);

    MatrixT q(1, 2);
    q << 0.1, 0.1;
    // Should not throw, just use all 3 samples
    VectorT pred = knn.predict(q);
    EXPECT_DOUBLE_EQ(pred(0), 0.0);  // majority is class 0
}

// ===================================================================
// Feature dimension mismatch on predict
// ===================================================================

TEST(KNNTest, PredictDimensionMismatch) {
    MatrixT X(5, 2);
    X.setRandom();
    VectorT y(5);
    y.setZero();

    KNNd knn(3);
    knn.fit(X, y);

    MatrixT q(1, 3);  // wrong number of features
    q.setRandom();
    EXPECT_THROW(knn.predict(q), std::invalid_argument);
}

// ===================================================================
// Setter tests
// ===================================================================

TEST(KNNTest, Setters) {
    KNNd knn;

    knn.set_k(7);
    EXPECT_EQ(knn.k(), 7);
    EXPECT_THROW(knn.set_k(0), std::invalid_argument);

    knn.set_metric(DistanceMetric::MANHATTAN);
    EXPECT_EQ(knn.metric(), DistanceMetric::MANHATTAN);

    knn.set_task(TaskType::REGRESSION);
    EXPECT_EQ(knn.task(), TaskType::REGRESSION);

    knn.set_weight(WeightType::DISTANCE);
    EXPECT_EQ(knn.weight(), WeightType::DISTANCE);

    knn.set_minkowski_p(3.0);
    EXPECT_DOUBLE_EQ(knn.minkowski_p(), 3.0);
    EXPECT_THROW(knn.set_minkowski_p(0.5), std::invalid_argument);
}

// ===================================================================
// Summary string
// ===================================================================

TEST(KNNTest, Summary) {
    KNNd knn(3, DistanceMetric::EUCLIDEAN, TaskType::CLASSIFICATION);
    std::string s = knn.summary();
    EXPECT_NE(s.find("k=3"), std::string::npos);
    EXPECT_NE(s.find("euclidean"), std::string::npos);
    EXPECT_NE(s.find("classification"), std::string::npos);
    EXPECT_NE(s.find("fitted=false"), std::string::npos);
}

// ===================================================================
// predict_one
// ===================================================================

TEST(KNNTest, PredictOne) {
    auto [X, y] = make_classification_data();
    KNNd knn(3, DistanceMetric::EUCLIDEAN, TaskType::CLASSIFICATION);
    knn.fit(X, y);

    VectorT query(2);
    query << 0.0, 0.0;
    EXPECT_DOUBLE_EQ(knn.predict_one(query), 0.0);
}
