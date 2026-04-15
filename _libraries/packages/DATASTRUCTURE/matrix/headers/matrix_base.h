#pragma once
#include <memory>
#include <vector>
#include <cstddef>

namespace matrix {

// MatrixType: Dense, Sparse, Custom
enum class MatrixType { Dense, Sparse, Custom };

// Abstract base class for all matrix types
class MatrixBase {
public:
    virtual ~MatrixBase() = default;
    virtual size_t rows() const = 0;
    virtual size_t cols() const = 0;
    virtual MatrixType type() const = 0;
    // Matrix operations
    virtual std::unique_ptr<MatrixBase> add(const MatrixBase& other) const = 0;
    virtual std::unique_ptr<MatrixBase> multiply(const MatrixBase& other) const = 0;
    virtual std::unique_ptr<MatrixBase> transpose() const = 0;
};

} // namespace matrix
