#include "../headers/matrix_dense.h"
#include <stdexcept>
#include <algorithm>
#include <future>

namespace matrix {

DenseMatrix::DenseMatrix(size_t rows, size_t cols)
    : rows_(rows), cols_(cols), data_(rows * cols, 0.0) {}

DenseMatrix::DenseMatrix(const std::vector<double>& data, size_t rows, size_t cols)
    : rows_(rows), cols_(cols), data_(data) {
    if (data.size() != rows * cols) throw std::invalid_argument("Data size mismatch");
}

double& DenseMatrix::at(size_t r, size_t c) {
    return data_[r * cols_ + c];
}

double DenseMatrix::at(size_t r, size_t c) const {
    return data_[r * cols_ + c];
}

std::unique_ptr<MatrixBase> DenseMatrix::add(const MatrixBase& other) const {
    if (other.rows() != rows_ || other.cols() != cols_)
        throw std::invalid_argument("Matrix size mismatch");
    const DenseMatrix* o = dynamic_cast<const DenseMatrix*>(&other);
    if (!o) throw std::invalid_argument("Type mismatch");
    std::vector<double> result(data_);
    // Parallel add
    std::vector<std::future<void>> futures;
    size_t n = data_.size();
    size_t num_threads = std::thread::hardware_concurrency();
    size_t chunk = n / num_threads;
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk;
        size_t end = (t == num_threads - 1) ? n : (t + 1) * chunk;
        futures.push_back(std::async([&, start, end] {
            for (size_t i = start; i < end; ++i) {
                result[i] += o->data_[i];
            }
        }));
    }
    for (auto& f : futures) f.get();
    return std::make_unique<DenseMatrix>(result, rows_, cols_);
}

std::unique_ptr<MatrixBase> DenseMatrix::multiply(const MatrixBase& other) const {
    if (cols_ != other.rows())
        throw std::invalid_argument("Matrix size mismatch");
    const DenseMatrix* o = dynamic_cast<const DenseMatrix*>(&other);
    if (!o) throw std::invalid_argument("Type mismatch");
    std::vector<double> result(rows_ * o->cols_, 0.0);
    // Parallel multiply (row-wise)
    std::vector<std::future<void>> futures;
    for (size_t r = 0; r < rows_; ++r) {
        futures.push_back(std::async([&, r] {
            for (size_t c = 0; c < o->cols_; ++c) {
                double sum = 0.0;
                for (size_t k = 0; k < cols_; ++k) {
                    sum += at(r, k) * o->at(k, c);
                }
                result[r * o->cols_ + c] = sum;
            }
        }));
    }
    for (auto& f : futures) f.get();
    return std::make_unique<DenseMatrix>(result, rows_, o->cols_);
}

std::unique_ptr<MatrixBase> DenseMatrix::transpose() const {
    std::vector<double> result(data_.size());
    for (size_t r = 0; r < rows_; ++r)
        for (size_t c = 0; c < cols_; ++c)
            result[c * rows_ + r] = at(r, c);
    return std::make_unique<DenseMatrix>(result, cols_, rows_);
}

} // namespace matrix
