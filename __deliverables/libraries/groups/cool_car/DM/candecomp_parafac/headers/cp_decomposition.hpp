#ifndef COOLBOX__LIBRARIES_PACKAGES_DM_CANDECOMP_PARAFAC_HEADERS_CP_DECOMPOSITION_HPP
#define COOLBOX__LIBRARIES_PACKAGES_DM_CANDECOMP_PARAFAC_HEADERS_CP_DECOMPOSITION_HPP

#include "../../MATRIX/headers/matrix_dense.h"

#include <cstddef>
#include <limits>
#include <vector>
#include <memory>

namespace dm {
namespace candecomp_parafac {

// ─── CP Decomposition Result ──────────────────────────────────────────────────

// CP decomposition (CANDECOMP/PARAFAC) represents a 3D tensor as:
//   Tensor[i, j, k] ≈ Σ_r (A[i, r] * B[j, r] * C[k, r])
// where A, B, C are factor matrices and R is the rank.
struct CPDecomposition {
    mytrix::DenseMatrix factor_a;  // I × R matrix
    mytrix::DenseMatrix factor_b;  // J × R matrix
    mytrix::DenseMatrix factor_c;  // K × R matrix
    double              fit_error = 0.0;
    std::size_t         num_iterations = 0U;
};

// ─── Configuration ────────────────────────────────────────────────────────────

struct CPDecompositionConfig {
    std::size_t rank = 5U;                          // Number of components (R)
    std::size_t max_iterations = 100U;              // Maximum ALS iterations
    double      tolerance = 1e-6;                   // Convergence tolerance
    double      init_factor = 0.01;                 // Random initialization factor
    bool        verbose = false;                    // Print iteration info
};

// ─── CP Decomposition via ALS (Alternating Least Squares) ────────────────────

// Decompose a 3D tensor into R rank-1 tensors using ALS.
// The tensor is provided in matricized (unfolded) forms:
//   - tensor_I: I × (J*K) unfolding along mode I
//   - tensor_J: J × (I*K) unfolding along mode J
//   - tensor_K: K × (I*J) unfolding along mode K
// Tensor dimensions (I, J, K) must be consistent across the unfoldings.
CPDecomposition decompose_3d_tensor(
    const mytrix::DenseMatrix& tensor_I,
    const mytrix::DenseMatrix& tensor_J,
    const mytrix::DenseMatrix& tensor_K,
    std::size_t                dim_I,
    std::size_t                dim_J,
    std::size_t                dim_K,
    const CPDecompositionConfig& config = CPDecompositionConfig()
);

// Helper: Compute Kronecker product of two matrices.
// Result is (A.rows() * B.rows()) × (A.cols() * B.cols())
mytrix::DenseMatrix kronecker_product(const mytrix::DenseMatrix& A,
                                      const mytrix::DenseMatrix& B);

// Helper: Element-wise matrix multiplication (Hadamard product).
mytrix::DenseMatrix hadamard_product(const mytrix::DenseMatrix& A,
                                     const mytrix::DenseMatrix& B);

// Helper: Compute Frobenius norm of a tensor given its unfoldings.
double tensor_frobenius_norm(const mytrix::DenseMatrix& tensor_unfolding);

} // namespace candecomp_parafac
} // namespace dm

#endif // COOLBOX__LIBRARIES_PACKAGES_DM_CANDECOMP_PARAFAC_HEADERS_CP_DECOMPOSITION_HPP
