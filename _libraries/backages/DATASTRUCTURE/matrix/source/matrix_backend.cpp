#include "../headers/matrix_backend.h"
#include "matrix_dense.h"
#include "matrix_sparse.h"
#include <memory>
#include <string>

namespace matrix {

// CPU backend implementation
class CpuBackend : public MatrixBackend {
public:
    BackendType backend_type() const override { return BackendType::CPU; }
    std::string name() const override { return "CPU"; }
    std::unique_ptr<MatrixBase> multiply(const MatrixBase& a, const MatrixBase& b) const override {
        // Try dense first
        if (a.type() == MatrixType::Dense && b.type() == MatrixType::Dense) {
            return static_cast<const DenseMatrix&>(a).multiply(b);
        }
        // Try sparse
        if (a.type() == MatrixType::Sparse && b.type() == MatrixType::Sparse) {
            return static_cast<const SparseMatrix&>(a).multiply(b);
        }
        throw std::invalid_argument("Unsupported matrix types for CPU multiply");
    }
};

// TODO: CUDA/OpenCL backend stubs
class CudaBackend : public MatrixBackend {
public:
    BackendType backend_type() const override { return BackendType::CUDA; }
    std::string name() const override { return "CUDA (stub)"; }
    std::unique_ptr<MatrixBase> multiply(const MatrixBase&, const MatrixBase&) const override {
        throw std::runtime_error("CUDA backend not implemented");
    }
};

class OpenClBackend : public MatrixBackend {
public:
    BackendType backend_type() const override { return BackendType::OpenCL; }
    std::string name() const override { return "OpenCL (stub)"; }
    std::unique_ptr<MatrixBase> multiply(const MatrixBase&, const MatrixBase&) const override {
        throw std::runtime_error("OpenCL backend not implemented");
    }
};

std::unique_ptr<MatrixBackend> create_best_backend() {
    // TODO: Detect CUDA/OpenCL at runtime
    return std::make_unique<CpuBackend>();
}

} // namespace matrix
