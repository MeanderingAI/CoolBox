#pragma once
#include "matrix_base.h"
#include <vector>
#include <thread>
#include <mutex>

namespace matrix {

class DenseMatrix : public MatrixBase {
public:
    DenseMatrix(size_t rows, size_t cols);
    DenseMatrix(const std::vector<double>& data, size_t rows, size_t cols);
    size_t rows() const override { return rows_; }
    size_t cols() const override { return cols_; }
    MatrixType type() const override { return MatrixType::Dense; }
    std::unique_ptr<MatrixBase> add(const MatrixBase& other) const override;
    std::unique_ptr<MatrixBase> multiply(const MatrixBase& other) const override;
    std::unique_ptr<MatrixBase> transpose() const override;
    double& at(size_t r, size_t c);
    double at(size_t r, size_t c) const;
    const std::vector<double>& data() const { return data_; }
private:
    size_t rows_, cols_;
    std::vector<double> data_;
};

} // namespace matrix
