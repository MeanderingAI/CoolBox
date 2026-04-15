#pragma once
#include "matrix_dense.h"
#include "matrix_sparse.h"
#include <memory>

namespace matrix {

// Factory for identity matrix (dense)
std::unique_ptr<DenseMatrix> identity_dense(size_t n);

// Factory for identity matrix (sparse)
std::unique_ptr<SparseMatrix> identity_sparse(size_t n);

// Factory for dense matrix with random values in [min, max]
std::unique_ptr<DenseMatrix> random_dense(size_t rows, size_t cols, double min = 0.0, double max = 1.0);

// Factory for sparse matrix with random nonzero entries (density in [0,1])
std::unique_ptr<SparseMatrix> random_sparse(size_t rows, size_t cols, double density = 0.1, double min = 0.0, double max = 1.0);

// Factory for dense matrix of zeros
std::unique_ptr<DenseMatrix> zeros_dense(size_t rows, size_t cols);

// Factory for dense matrix of ones
std::unique_ptr<DenseMatrix> ones_dense(size_t rows, size_t cols);

// Factory for diagonal dense matrix
std::unique_ptr<DenseMatrix> diagonal_dense(const std::vector<double>& diag);

// Factory for diagonal sparse matrix
std::unique_ptr<SparseMatrix> diagonal_sparse(const std::vector<double>& diag);

} // namespace matrix
