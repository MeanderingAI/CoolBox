#include "cp_decomposition.hpp"

#include <tyst_framework.hpp>

#include <cmath>

namespace {

using namespace dm::candecomp_parafac;

// ─── Kronecker Product ────────────────────────────────────────────────────────

TYST_TEST(CandecompParafacTest, KroneckerProductDimensions) {
    mytrix::DenseMatrix A(2, 3);
    mytrix::DenseMatrix B(2, 2);

    mytrix::DenseMatrix result = kronecker_product(A, B);

    TYST_ASSERT_EQ(result.rows(), 4);
    TYST_ASSERT_EQ(result.cols(), 6);
}

TYST_TEST(CandecompParafacTest, KroneckerProductValues) {
    mytrix::DenseMatrix A(2, 2);
    A(0, 0) = 1.0;
    A(0, 1) = 2.0;
    A(1, 0) = 3.0;
    A(1, 1) = 4.0;

    mytrix::DenseMatrix B(2, 2);
    B(0, 0) = 5.0;
    B(0, 1) = 6.0;
    B(1, 0) = 7.0;
    B(1, 1) = 8.0;

    mytrix::DenseMatrix result = kronecker_product(A, B);

    TYST_EXPECT_NEAR(result(0, 0), 1.0 * 5.0, 1e-12);
    TYST_EXPECT_NEAR(result(0, 1), 1.0 * 6.0, 1e-12);
    TYST_EXPECT_NEAR(result(2, 0), 3.0 * 5.0, 1e-12);
    TYST_EXPECT_NEAR(result(2, 1), 3.0 * 6.0, 1e-12);
}

// ─── Hadamard Product ─────────────────────────────────────────────────────────

TYST_TEST(CandecompParafacTest, HadamardProductValues) {
    mytrix::DenseMatrix A(2, 2);
    A(0, 0) = 1.0;
    A(0, 1) = 2.0;
    A(1, 0) = 3.0;
    A(1, 1) = 4.0;

    mytrix::DenseMatrix B(2, 2);
    B(0, 0) = 5.0;
    B(0, 1) = 6.0;
    B(1, 0) = 7.0;
    B(1, 1) = 8.0;

    mytrix::DenseMatrix result = hadamard_product(A, B);

    TYST_EXPECT_NEAR(result(0, 0), 1.0 * 5.0, 1e-12);
    TYST_EXPECT_NEAR(result(0, 1), 2.0 * 6.0, 1e-12);
    TYST_EXPECT_NEAR(result(1, 0), 3.0 * 7.0, 1e-12);
    TYST_EXPECT_NEAR(result(1, 1), 4.0 * 8.0, 1e-12);
}

// ─── Frobenius Norm ───────────────────────────────────────────────────────────

TYST_TEST(CandecompParafacTest, TensorFrobeniusNorm) {
    mytrix::DenseMatrix tensor(3, 4);
    tensor(0, 0) = 1.0;  tensor(0, 1) = 2.0;  tensor(0, 2) = 3.0;  tensor(0, 3) = 4.0;
    tensor(1, 0) = 5.0;  tensor(1, 1) = 6.0;  tensor(1, 2) = 7.0;  tensor(1, 3) = 8.0;
    tensor(2, 0) = 9.0;  tensor(2, 1) = 10.0; tensor(2, 2) = 11.0; tensor(2, 3) = 12.0;

    double norm = tensor_frobenius_norm(tensor);

    // sqrt(1^2 + 2^2 + ... + 12^2) = sqrt(650) ≈ 25.495
    TYST_EXPECT_NEAR(norm, std::sqrt(650.0), 1e-6);
}

// ─── Decompose Simple 3D Tensor ───────────────────────────────────────────────

TYST_TEST(CandecompParafacTest, FactorMatrixDimensions2x2x2) {
    // 2×2×2 tensor unfoldings
    mytrix::DenseMatrix tensor_I(2, 4);
    mytrix::DenseMatrix tensor_J(2, 4);
    mytrix::DenseMatrix tensor_K(2, 4);

    // Fill with a simple rank-1 pattern
    tensor_I(0, 0) = 1.0;   tensor_I(0, 1) = 0.5;  tensor_I(0, 2) = 0.5;  tensor_I(0, 3) = 0.25;
    tensor_I(1, 0) = 0.5;   tensor_I(1, 1) = 0.25; tensor_I(1, 2) = 0.25; tensor_I(1, 3) = 0.125;

    tensor_J(0, 0) = 1.0;   tensor_J(0, 1) = 0.5;  tensor_J(0, 2) = 0.5;  tensor_J(0, 3) = 0.25;
    tensor_J(1, 0) = 0.5;   tensor_J(1, 1) = 0.25; tensor_J(1, 2) = 0.25; tensor_J(1, 3) = 0.125;

    tensor_K(0, 0) = 1.0;   tensor_K(0, 1) = 0.5;  tensor_K(0, 2) = 0.5;  tensor_K(0, 3) = 0.25;
    tensor_K(1, 0) = 0.5;   tensor_K(1, 1) = 0.25; tensor_K(1, 2) = 0.25; tensor_K(1, 3) = 0.125;

    CPDecompositionConfig config;
    config.rank = 1;
    config.max_iterations = 50;
    config.tolerance = 1e-6;
    config.verbose = false;

    CPDecomposition decomp = decompose_3d_tensor(
        tensor_I, tensor_J, tensor_K, 2, 2, 2, config);

    TYST_ASSERT_EQ(decomp.factor_a.rows(), 2);
    TYST_ASSERT_EQ(decomp.factor_a.cols(), 1);
    TYST_ASSERT_EQ(decomp.factor_b.rows(), 2);
    TYST_ASSERT_EQ(decomp.factor_b.cols(), 1);
    TYST_ASSERT_EQ(decomp.factor_c.rows(), 2);
    TYST_ASSERT_EQ(decomp.factor_c.cols(), 1);
}

TYST_TEST(CandecompParafacTest, FactorMatrixDimensions3x4x3WithRank3) {
    // 3×4×3 tensor with rank 3
    mytrix::DenseMatrix tensor_I(3, 12);  // 3×(4*3)
    mytrix::DenseMatrix tensor_J(4, 9);   // 4×(3*3)
    mytrix::DenseMatrix tensor_K(3, 12);  // 3×(3*4)

    for (int i = 0; i < tensor_I.rows(); ++i)
        for (int j = 0; j < tensor_I.cols(); ++j)
            tensor_I(i, j) = 0.1;

    for (int i = 0; i < tensor_J.rows(); ++i)
        for (int j = 0; j < tensor_J.cols(); ++j)
            tensor_J(i, j) = 0.1;

    for (int i = 0; i < tensor_K.rows(); ++i)
        for (int j = 0; j < tensor_K.cols(); ++j)
            tensor_K(i, j) = 0.1;

    CPDecompositionConfig config;
    config.rank = 3;
    config.max_iterations = 10;

    CPDecomposition decomp = decompose_3d_tensor(
        tensor_I, tensor_J, tensor_K, 3, 4, 3, config);

    TYST_ASSERT_EQ(decomp.factor_a.rows(), 3);
    TYST_ASSERT_EQ(decomp.factor_a.cols(), 3);
    TYST_ASSERT_EQ(decomp.factor_b.rows(), 4);
    TYST_ASSERT_EQ(decomp.factor_b.cols(), 3);
    TYST_ASSERT_EQ(decomp.factor_c.rows(), 3);
    TYST_ASSERT_EQ(decomp.factor_c.cols(), 3);
}


} // namespace

// Main entry point for tyst framework
#if defined(__APPLE__) && defined(__aarch64__)
#endif
