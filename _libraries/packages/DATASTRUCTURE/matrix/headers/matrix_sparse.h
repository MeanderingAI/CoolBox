#pragma once
#include "matrix_base.h"
#include <unordered_map>
#include <tuple>

namespace matrix {

struct SparseMatrixKeyHash {
    size_t operator()(const std::tuple<size_t, size_t>& key) const noexcept {
        const auto first = std::get<0>(key);
        const auto second = std::get<1>(key);
        return std::hash<size_t>{}(first) ^ (std::hash<size_t>{}(second) << 1);
    }
};

// Simple CSR-like sparse matrix
class SparseMatrix : public MatrixBase {
public:
    SparseMatrix(size_t rows, size_t cols);
    size_t rows() const override { return rows_; }
    size_t cols() const override { return cols_; }
    MatrixType type() const override { return MatrixType::Sparse; }
    std::unique_ptr<MatrixBase> add(const MatrixBase& other) const override;
    std::unique_ptr<MatrixBase> multiply(const MatrixBase& other) const override;
    std::unique_ptr<MatrixBase> transpose() const override;
    void set(size_t r, size_t c, double value);
    double get(size_t r, size_t c) const;
private:
    size_t rows_, cols_;
    // (row, col) -> value
    std::unordered_map<std::tuple<size_t, size_t>, double, SparseMatrixKeyHash> data_;
};

} // namespace matrix
