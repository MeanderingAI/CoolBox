#pragma once
#include "matrix_base.h"
#include <memory>
#include <string>

namespace matrix {

// Backend types
enum class BackendType { CPU, CUDA, OpenCL };

// Abstract backend interface
class MatrixBackend {
public:
    virtual ~MatrixBackend() = default;
    virtual BackendType backend_type() const = 0;
    virtual std::string name() const = 0;
    virtual std::unique_ptr<MatrixBase> multiply(const MatrixBase& a, const MatrixBase& b) const = 0;
    // Add more operations as needed
};

// Factory for backend selection
std::unique_ptr<MatrixBackend> create_best_backend();

} // namespace matrix
