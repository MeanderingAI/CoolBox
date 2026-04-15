#include "../headers/matrix_sparse.h"
#include <stdexcept>

namespace matrix {

SparseMatrix::SparseMatrix(size_t rows, size_t cols)
    : rows_(rows), cols_(cols) {}

void SparseMatrix::set(size_t r, size_t c, double value) {
    data_[std::make_tuple(r, c)] = value;
}

double SparseMatrix::get(size_t r, size_t c) const {
    auto it = data_.find(std::make_tuple(r, c));
    return (it != data_.end()) ? it->second : 0.0;
}

std::unique_ptr<MatrixBase> SparseMatrix::add(const MatrixBase& other) const {
    if (other.rows() != rows_ || other.cols() != cols_)
        throw std::invalid_argument("Matrix size mismatch");
    const SparseMatrix* o = dynamic_cast<const SparseMatrix*>(&other);
    if (!o) throw std::invalid_argument("Type mismatch");
    auto result = std::make_unique<SparseMatrix>(rows_, cols_);
    result->data_ = data_;
    for (const auto& kv : o->data_) {
        result->data_[kv.first] += kv.second;
    }
    return result;
}

std::unique_ptr<MatrixBase> SparseMatrix::multiply(const MatrixBase& other) const {
    if (cols_ != other.rows())
        throw std::invalid_argument("Matrix size mismatch");
    const SparseMatrix* o = dynamic_cast<const SparseMatrix*>(&other);
    if (!o) throw std::invalid_argument("Type mismatch");
    auto result = std::make_unique<SparseMatrix>(rows_, o->cols_);
    for (const auto& [key, val] : data_) {
        size_t r, k;
        std::tie(r, k) = key;
        for (size_t c = 0; c < o->cols_; ++c) {
            double v = o->get(k, c);
            if (v != 0.0)
                result->data_[std::make_tuple(r, c)] += val * v;
        }
    }
    return result;
}

std::unique_ptr<MatrixBase> SparseMatrix::transpose() const {
    auto result = std::make_unique<SparseMatrix>(cols_, rows_);
    for (const auto& [key, val] : data_) {
        size_t r, c;
        std::tie(r, c) = key;
        result->set(c, r, val);
    }
    return result;
}

} // namespace matrix
