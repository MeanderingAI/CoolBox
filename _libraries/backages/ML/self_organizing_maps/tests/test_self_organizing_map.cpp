#include <gtest/gtest.h>
#include "self_organizing_map.h"

#include <Eigen/Dense>
#include <cmath>

using namespace ml;
using SOM = SelfOrganizingMap<double>;
using MatrixT = SOM::MatrixT;
using VectorT = SOM::VectorT;

static constexpr double TOL = 1e-4;

// ===================================================================
// Helper: generate 2D cluster data
// ===================================================================
static MatrixT make_clusters() {
    // 3 clusters at (0,0), (5,0), (0,5) — 30 points each
    MatrixT data(90, 2);
    std::mt19937 rng(123);
    std::normal_distribution<double> noise(0, 0.3);

    for (int i = 0; i < 30; ++i) {
        data(i, 0)      = 0 + noise(rng);
        data(i, 1)      = 0 + noise(rng);
        data(30 + i, 0) = 5 + noise(rng);
        data(30 + i, 1) = 0 + noise(rng);
        data(60 + i, 0) = 0 + noise(rng);
        data(60 + i, 1) = 5 + noise(rng);
    }
    return data;
}

// ===================================================================
// Construction tests
// ===================================================================

TEST(SOMTest, ConstructionValid) {
    SOM som(5, 5, 3);
    EXPECT_EQ(som.rows(), 5);
    EXPECT_EQ(som.cols(), 5);
    EXPECT_EQ(som.input_dim(), 3);
    EXPECT_EQ(som.num_neurons(), 25);
}

TEST(SOMTest, ConstructionInvalidThrows) {
    EXPECT_THROW(SOM(0, 5, 3), std::invalid_argument);
    EXPECT_THROW(SOM(5, 0, 3), std::invalid_argument);
    EXPECT_THROW(SOM(5, 5, 0), std::invalid_argument);
}

// ===================================================================
// Initialisation tests
// ===================================================================

TEST(SOMTest, InitRandomUniform) {
    SOM som(3, 3, 2);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::RANDOM_UNIFORM);

    // Weights should be within data range
    double min_x = data.col(0).minCoeff();
    double max_x = data.col(0).maxCoeff();
    for (int i = 0; i < som.num_neurons(); ++i) {
        EXPECT_GE(som.weights()(i, 0), min_x - TOL);
        EXPECT_LE(som.weights()(i, 0), max_x + TOL);
    }
}

TEST(SOMTest, InitRandomSample) {
    SOM som(3, 3, 2);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::RANDOM_SAMPLE);

    // Each weight should match some data row
    for (int i = 0; i < som.num_neurons(); ++i) {
        bool found = false;
        for (int j = 0; j < data.rows(); ++j) {
            if ((som.weights().row(i) - data.row(j)).norm() < TOL) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Weight " << i << " not found in data";
    }
}

TEST(SOMTest, InitPCA) {
    SOM som(3, 3, 2);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::PCA);

    // Weights should not all be zero
    EXPECT_GT(som.weights().norm(), 0);
}

TEST(SOMTest, InitDimensionMismatchThrows) {
    SOM som(3, 3, 2);
    MatrixT bad_data(10, 5);  // 5 features, SOM expects 2
    EXPECT_THROW(som.initialize(bad_data), std::invalid_argument);
}

// ===================================================================
// BMU tests
// ===================================================================

TEST(SOMTest, FindBMU) {
    SOM som(2, 2, 2);
    // Manually set weights to known positions
    som.weights().row(0) << 0, 0;
    som.weights().row(1) << 10, 0;
    som.weights().row(2) << 0, 10;
    som.weights().row(3) << 10, 10;

    VectorT sample(2);

    // Closest to (0,0)
    sample << 0.1, 0.1;
    BMUResult bmu = som.find_bmu(sample);
    EXPECT_EQ(bmu.row, 0);
    EXPECT_EQ(bmu.col, 0);

    // Closest to (10,10)
    sample << 9.5, 9.5;
    bmu = som.find_bmu(sample);
    EXPECT_EQ(bmu.row, 1);
    EXPECT_EQ(bmu.col, 1);

    // Closest to (10,0)
    sample << 8, 1;
    bmu = som.find_bmu(sample);
    EXPECT_EQ(bmu.row, 0);
    EXPECT_EQ(bmu.col, 1);
}

// ===================================================================
// Training tests
// ===================================================================

TEST(SOMTest, FitReducesQuantisationError) {
    SOM som(5, 5, 2, 0.5, -1, NeighbourhoodType::GAUSSIAN, 42);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::RANDOM_UNIFORM);

    double error_before = som.quantisation_error(data);
    som.fit(data, 50);
    double error_after = som.quantisation_error(data);

    EXPECT_LT(error_after, error_before)
        << "Quantisation error should decrease after training";
}

TEST(SOMTest, FitBubbleNeighbourhood) {
    SOM som(5, 5, 2, 0.5, -1, NeighbourhoodType::BUBBLE, 42);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::RANDOM_UNIFORM);

    double error_before = som.quantisation_error(data);
    som.fit(data, 50);
    double error_after = som.quantisation_error(data);

    EXPECT_LT(error_after, error_before);
}

TEST(SOMTest, FitDimensionMismatchThrows) {
    SOM som(3, 3, 2);
    MatrixT bad_data(10, 5);
    EXPECT_THROW(som.fit(bad_data), std::invalid_argument);
}

// ===================================================================
// Transform tests
// ===================================================================

TEST(SOMTest, TransformReturnsCorrectShape) {
    SOM som(4, 4, 2, 0.5, -1, NeighbourhoodType::GAUSSIAN, 42);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::RANDOM_UNIFORM);
    som.fit(data, 20);

    Eigen::MatrixXi assignments = som.transform(data);
    EXPECT_EQ(assignments.rows(), data.rows());
    EXPECT_EQ(assignments.cols(), 2);

    // All assignments should be within grid bounds
    for (int i = 0; i < assignments.rows(); ++i) {
        EXPECT_GE(assignments(i, 0), 0);
        EXPECT_LT(assignments(i, 0), som.rows());
        EXPECT_GE(assignments(i, 1), 0);
        EXPECT_LT(assignments(i, 1), som.cols());
    }
}

TEST(SOMTest, ClustersMapToDifferentRegions) {
    SOM som(10, 10, 2, 0.5, -1, NeighbourhoodType::GAUSSIAN, 42);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::RANDOM_UNIFORM);
    som.fit(data, 100);

    Eigen::MatrixXi assignments = som.transform(data);

    // Average grid position for each cluster
    auto avg_pos = [&](int start, int end) -> std::pair<double, double> {
        double r = 0, c = 0;
        for (int i = start; i < end; ++i) {
            r += assignments(i, 0);
            c += assignments(i, 1);
        }
        int n = end - start;
        return {r / n, c / n};
    };

    auto [r1, c1] = avg_pos(0, 30);
    auto [r2, c2] = avg_pos(30, 60);
    auto [r3, c3] = avg_pos(60, 90);

    // Clusters should map to distinct grid regions
    double d12 = std::sqrt((r1-r2)*(r1-r2) + (c1-c2)*(c1-c2));
    double d13 = std::sqrt((r1-r3)*(r1-r3) + (c1-c3)*(c1-c3));
    double d23 = std::sqrt((r2-r3)*(r2-r3) + (c2-c3)*(c2-c3));

    EXPECT_GT(d12, 1.0) << "Clusters 1 and 2 should be separated on the grid";
    EXPECT_GT(d13, 1.0) << "Clusters 1 and 3 should be separated on the grid";
    EXPECT_GT(d23, 1.0) << "Clusters 2 and 3 should be separated on the grid";
}

// ===================================================================
// U-Matrix tests
// ===================================================================

TEST(SOMTest, UMatrixShape) {
    SOM som(4, 6, 2);
    MatrixT data = make_clusters();
    som.initialize(data);

    MatrixT umat = som.u_matrix();
    EXPECT_EQ(umat.rows(), 4);
    EXPECT_EQ(umat.cols(), 6);
}

TEST(SOMTest, UMatrixNonNegative) {
    SOM som(5, 5, 2, 0.5, -1, NeighbourhoodType::GAUSSIAN, 42);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::RANDOM_UNIFORM);
    som.fit(data, 50);

    MatrixT umat = som.u_matrix();
    for (int r = 0; r < umat.rows(); ++r)
        for (int c = 0; c < umat.cols(); ++c)
            EXPECT_GE(umat(r, c), 0.0);
}

// ===================================================================
// Topographic error tests
// ===================================================================

TEST(SOMTest, TopographicErrorRange) {
    SOM som(5, 5, 2, 0.5, -1, NeighbourhoodType::GAUSSIAN, 42);
    MatrixT data = make_clusters();
    som.initialize(data, InitStrategy::RANDOM_UNIFORM);
    som.fit(data, 50);

    double te = som.topographic_error(data);
    EXPECT_GE(te, 0.0);
    EXPECT_LE(te, 1.0);
}

// ===================================================================
// Accessor tests
// ===================================================================

TEST(SOMTest, WeightAccess) {
    SOM som(3, 4, 2);
    som.weights().setOnes();

    VectorT w = som.weight(1, 2);
    EXPECT_NEAR(w(0), 1.0, TOL);
    EXPECT_NEAR(w(1), 1.0, TOL);
}

TEST(SOMTest, GridPosition) {
    SOM som(3, 4, 2);
    auto [r, c] = som.grid_position(7);  // row=1, col=3 for 4-wide grid
    EXPECT_EQ(r, 1);
    EXPECT_EQ(c, 3);
}

// ===================================================================
// 1x1 edge case
// ===================================================================

TEST(SOMTest, SingleNeuron) {
    SOM som(1, 1, 3);
    MatrixT data(5, 3);
    data << 1, 2, 3,
            4, 5, 6,
            7, 8, 9,
            2, 3, 4,
            5, 6, 7;

    som.initialize(data, InitStrategy::RANDOM_UNIFORM);
    som.fit(data, 10);

    // All samples should map to (0,0)
    Eigen::MatrixXi assignments = som.transform(data);
    for (int i = 0; i < data.rows(); ++i) {
        EXPECT_EQ(assignments(i, 0), 0);
        EXPECT_EQ(assignments(i, 1), 0);
    }

    // Weight should be close to mean of data
    VectorT w = som.weight(0, 0);
    VectorT mean = data.colwise().mean().transpose();
    EXPECT_LT((w - mean).norm(), 2.0);
}
