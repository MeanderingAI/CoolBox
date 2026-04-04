#include "../headers/matrix_utils.h"

namespace matrix {

std::unique_ptr<DenseMatrix> identity_dense(size_t n) {
    auto mat = std::make_unique<DenseMatrix>(n, n);
    for (size_t i = 0; i < n; ++i) mat->at(i, i) = 1.0;
    return mat;
}

std::unique_ptr<SparseMatrix> identity_sparse(size_t n) {
    auto mat = std::make_unique<SparseMatrix>(n, n);
    for (size_t i = 0; i < n; ++i) mat->set(i, i, 1.0);
    return mat;
}


#include <random>

std::unique_ptr<DenseMatrix> random_dense(size_t rows, size_t cols, double min, double max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(min, max);
    std::vector<double> data(rows * cols);
    for (auto& v : data) v = dist(gen);
    return std::make_unique<DenseMatrix>(data, rows, cols);
}

std::unique_ptr<SparseMatrix> random_sparse(size_t rows, size_t cols, double density, double min, double max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(min, max);
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    auto mat = std::make_unique<SparseMatrix>(rows, cols);
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            if (prob(gen) < density) {
                mat->set(r, c, dist(gen));
            }
        }
    }
    return mat;
}

std::unique_ptr<DenseMatrix> zeros_dense(size_t rows, size_t cols) {
    return std::make_unique<DenseMatrix>(std::vector<double>(rows * cols, 0.0), rows, cols);
}

std::unique_ptr<DenseMatrix> ones_dense(size_t rows, size_t cols) {
    return std::make_unique<DenseMatrix>(std::vector<double>(rows * cols, 1.0), rows, cols);
}

std::unique_ptr<DenseMatrix> diagonal_dense(const std::vector<double>& diag) {
    size_t n = diag.size();
    auto mat = std::make_unique<DenseMatrix>(n, n);
    for (size_t i = 0; i < n; ++i) mat->at(i, i) = diag[i];
    return mat;
}

std::unique_ptr<SparseMatrix> diagonal_sparse(const std::vector<double>& diag) {
    size_t n = diag.size();
    auto mat = std::make_unique<SparseMatrix>(n, n);
    for (size_t i = 0; i < n; ++i) mat->set(i, i, diag[i]);
    return mat;
}

} // namespace matrix
