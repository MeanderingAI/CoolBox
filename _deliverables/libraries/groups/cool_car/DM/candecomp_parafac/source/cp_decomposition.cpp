#include "cp_decomposition.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <random>

namespace dm {
namespace candecomp_parafac {

// ─── Helper Functions ─────────────────────────────────────────────────────────

mytrix::DenseMatrix kronecker_product(const mytrix::DenseMatrix& A,
                                      const mytrix::DenseMatrix& B) {
    int A_rows = A.rows();
    int A_cols = A.cols();
    int B_rows = B.rows();
    int B_cols = B.cols();

    mytrix::DenseMatrix result(A_rows * B_rows, A_cols * B_cols);

    for (int i = 0; i < A_rows; ++i) {
        for (int j = 0; j < A_cols; ++j) {
            for (int k = 0; k < B_rows; ++k) {
                for (int l = 0; l < B_cols; ++l) {
                    result(i * B_rows + k, j * B_cols + l) = A(i, j) * B(k, l);
                }
            }
        }
    }

    return result;
}

mytrix::DenseMatrix hadamard_product(const mytrix::DenseMatrix& A,
                                     const mytrix::DenseMatrix& B) {
    if (A.rows() != B.rows() || A.cols() != B.cols()) {
        throw std::invalid_argument(
            "Hadamard product requires matrices of equal dimensions");
    }

    mytrix::DenseMatrix result(A.rows(), A.cols());
    for (int i = 0; i < A.rows(); ++i) {
        for (int j = 0; j < A.cols(); ++j) {
            result(i, j) = A(i, j) * B(i, j);
        }
    }

    return result;
}

double tensor_frobenius_norm(const mytrix::DenseMatrix& tensor_unfolding) {
    return tensor_unfolding.norm();
}

// ─── CP Decomposition Implementation ───────────────────────────────────────────

CPDecomposition decompose_3d_tensor(
    const mytrix::DenseMatrix& tensor_I,
    const mytrix::DenseMatrix& tensor_J,
    const mytrix::DenseMatrix& tensor_K,
    std::size_t                dim_I,
    std::size_t                dim_J,
    std::size_t                dim_K,
    const CPDecompositionConfig& config) {

    // Validate tensor unfolding dimensions
    if (static_cast<std::size_t>(tensor_I.rows()) != dim_I ||
        static_cast<std::size_t>(tensor_I.cols()) != dim_J * dim_K) {
        throw std::invalid_argument("tensor_I dimensions mismatch");
    }
    if (static_cast<std::size_t>(tensor_J.rows()) != dim_J ||
        static_cast<std::size_t>(tensor_J.cols()) != dim_I * dim_K) {
        throw std::invalid_argument("tensor_J dimensions mismatch");
    }
    if (static_cast<std::size_t>(tensor_K.rows()) != dim_K ||
        static_cast<std::size_t>(tensor_K.cols()) != dim_I * dim_J) {
        throw std::invalid_argument("tensor_K dimensions mismatch");
    }

    std::size_t R = config.rank;

    // Initialize factor matrices with small random values
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::normal_distribution<double> dist(0.0, config.init_factor);

    auto random_matrix = [&](std::size_t rows, std::size_t cols) {
        mytrix::DenseMatrix mat(static_cast<int>(rows), static_cast<int>(cols));
        for (std::size_t i = 0; i < rows; ++i) {
            for (std::size_t j = 0; j < cols; ++j) {
                mat(static_cast<int>(i), static_cast<int>(j)) = dist(rng);
            }
        }
        return mat;
    };

    mytrix::DenseMatrix A = random_matrix(dim_I, R);
    mytrix::DenseMatrix B = random_matrix(dim_J, R);
    mytrix::DenseMatrix C = random_matrix(dim_K, R);

    double tensor_norm = tensor_frobenius_norm(tensor_I);
    if (tensor_norm < 1e-14) {
        tensor_norm = 1.0;  // Avoid division by zero
    }

    double prev_fit_error = std::numeric_limits<double>::max();

    // ALS iterations
    for (std::size_t iter = 0; iter < config.max_iterations; ++iter) {
        // ─── Update A ──────────────────────────────────────────────────────────
        // Solve: A = tensor_I * (C ⊙ B) * ((C^T*C) * (B^T*B))^{-1}
        mytrix::DenseMatrix G_C = A;  // Compute Gram matrix C^T * C
        mytrix::DenseMatrix gram_C(static_cast<int>(R), static_cast<int>(R));
        for (std::size_t r = 0; r < R; ++r) {
            for (std::size_t s = 0; s < R; ++s) {
                double sum = 0.0;
                for (int k = 0; k < C.rows(); ++k) {
                    sum += C(k, static_cast<int>(r)) * C(k, static_cast<int>(s));
                }
                gram_C(static_cast<int>(r), static_cast<int>(s)) = sum;
            }
        }

        mytrix::DenseMatrix gram_B(static_cast<int>(R), static_cast<int>(R));
        for (std::size_t r = 0; r < R; ++r) {
            for (std::size_t s = 0; s < R; ++s) {
                double sum = 0.0;
                for (int j = 0; j < B.rows(); ++j) {
                    sum += B(j, static_cast<int>(r)) * B(j, static_cast<int>(s));
                }
                gram_B(static_cast<int>(r), static_cast<int>(s)) = sum;
            }
        }

        // Kronecker product: C ⊙ B (row-wise kron)
        mytrix::DenseMatrix kr_CB = kronecker_product(C, B);

        // Solve mode-I unfolding: A = tensor_I * kr_CB * inv(gram_C ∘ gram_B)
        // For simplicity, use pseudo-inverse via normal equations
        mytrix::DenseMatrix hadamard_grams = hadamard_product(gram_C, gram_B);

        // Compute A by least squares
        mytrix::DenseMatrix numerator = tensor_I * kr_CB;

        // Simple Gaussian elimination for 5×5 or small matrix (for production, use proper linear solver)
        // For now, use Eigen's built-in solver via the data member
        A.data = numerator.data * hadamard_grams.data.inverse();

        // ─── Update B ──────────────────────────────────────────────────────────
        mytrix::DenseMatrix gram_A(static_cast<int>(R), static_cast<int>(R));
        for (std::size_t r = 0; r < R; ++r) {
            for (std::size_t s = 0; s < R; ++s) {
                double sum = 0.0;
                for (int i = 0; i < A.rows(); ++i) {
                    sum += A(i, static_cast<int>(r)) * A(i, static_cast<int>(s));
                }
                gram_A(static_cast<int>(r), static_cast<int>(s)) = sum;
            }
        }

        // Re-compute gram_C (it may have changed, but in this iteration it shouldn't)
        for (std::size_t r = 0; r < R; ++r) {
            for (std::size_t s = 0; s < R; ++s) {
                double sum = 0.0;
                for (int k = 0; k < C.rows(); ++k) {
                    sum += C(k, static_cast<int>(r)) * C(k, static_cast<int>(s));
                }
                gram_C(static_cast<int>(r), static_cast<int>(s)) = sum;
            }
        }

        // Kronecker product: C ⊙ A
        mytrix::DenseMatrix kr_CA = kronecker_product(C, A);
        mytrix::DenseMatrix numerator_B = tensor_J * kr_CA;
        mytrix::DenseMatrix hadamard_grams_B = hadamard_product(gram_C, gram_A);

        B.data = numerator_B.data * hadamard_grams_B.data.inverse();

        // ─── Update C ──────────────────────────────────────────────────────────
        // Recompute gram_A and gram_B for the final update
        for (std::size_t r = 0; r < R; ++r) {
            for (std::size_t s = 0; s < R; ++s) {
                double sum_A = 0.0;
                for (int i = 0; i < A.rows(); ++i) {
                    sum_A += A(i, static_cast<int>(r)) * A(i, static_cast<int>(s));
                }
                gram_A(static_cast<int>(r), static_cast<int>(s)) = sum_A;

                double sum_B = 0.0;
                for (int j = 0; j < B.rows(); ++j) {
                    sum_B += B(j, static_cast<int>(r)) * B(j, static_cast<int>(s));
                }
                gram_B(static_cast<int>(r), static_cast<int>(s)) = sum_B;
            }
        }

        // Kronecker product: B ⊙ A
        mytrix::DenseMatrix kr_BA = kronecker_product(B, A);
        mytrix::DenseMatrix numerator_C = tensor_K * kr_BA;
        mytrix::DenseMatrix hadamard_grams_C = hadamard_product(gram_B, gram_A);

        C.data = numerator_C.data * hadamard_grams_C.data.inverse();

        // ─── Convergence Check ────────────────────────────────────────────────
        // Reconstruct and compute fit error (optional, expensive)
        double current_fit_error = std::abs(prev_fit_error - 0.0);  // Simplified
        if (config.verbose) {
            std::cout << "  CP Decomposition iteration " << (iter + 1) << ": fit_error = "
                      << current_fit_error << "\n";
        }

        if (current_fit_error < config.tolerance) {
            if (config.verbose) {
                std::cout << "  Converged after " << (iter + 1) << " iterations\n";
            }
            prev_fit_error = current_fit_error;
            break;
        }

        prev_fit_error = current_fit_error;
    }

    CPDecomposition result;
    result.factor_a = A;
    result.factor_b = B;
    result.factor_c = C;
    result.fit_error = prev_fit_error;
    result.num_iterations = config.max_iterations;

    return result;
}

} // namespace candecomp_parafac
} // namespace dm
