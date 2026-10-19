// Always include headers outside the namespace!
#pragma once
#include <complex>
#include <type_traits>
#include "backend_config_templated.h"
#include "matrix_base.h"
#include <Eigen/Dense>
#include <memory>
#include <stdexcept>
#include <vector>

// Conditionally include platform-specific backends
#ifdef MYTRIX_ENABLE_METAL
#include "metal_backend.h"
#endif

#ifdef MYTRIX_ENABLE_CUDA
#include "cuda_backend.h"
#endif

#ifdef MYTRIX_ENABLE_OPENCL
#include "opencl_backend.h"
#endif

namespace mytrix {

namespace detail {

inline mytrix::ComputeBackend resolve_operation_backend(const mytrix::OperationOptions& options) {
    const mytrix::ComputeBackend active = mytrix::BackendConfig::resolve_backend(options.backend, options.boost);
    mytrix::BackendConfig::set_active_backend(active);
    return active;
}

inline bool uses_cpu_fallback(mytrix::ComputeBackend backend) {
    return backend == mytrix::ComputeBackend::CPU ||
           backend == mytrix::ComputeBackend::BOOST ||
           backend == mytrix::ComputeBackend::EIGEN;
}

inline mytrix::OperationOptions default_operation_options() {
    return mytrix::OperationOptions{mytrix::BackendConfig::requested_backend(), mytrix::BackendConfig::boost_enabled()};
}

} // namespace detail

class DenseVector;


// --- DenseMatrix definition first ---

class DenseMatrix : public mytrix::MatrixBase {
public:
    // Set all elements to random values (Eigen API compatibility)
    void setRandom() { data.setRandom(); }
    Eigen::MatrixXd data;

    // Static methods for Eigen-like API
    static DenseMatrix Ones(int r, int c);
    static DenseMatrix Ones(std::size_t r, std::size_t c);
    static DenseMatrix Random(int r, int c);
    static DenseMatrix Random(std::size_t r, std::size_t c);

    DenseMatrix() : data(0, 0) {}
    DenseMatrix(int r, int c) : data(r, c) { data.setZero(); }
    // Implicit conversion from Eigen for test/interop compatibility
    DenseMatrix(const Eigen::MatrixXd& m) : data(m) {}
    DenseMatrix(Eigen::MatrixXd&& m) : data(std::move(m)) {}
    // Flat-vector constructor: data in row-major order
    DenseMatrix(const std::vector<double>& flat, int r, int c) : data(r, c) {
        for (int i = 0; i < r; ++i)
            for (int j = 0; j < c; ++j)
                data(i, j) = flat[static_cast<std::size_t>(i * c + j)];
    }
    DenseMatrix(const std::vector<double>& flat, std::size_t r, std::size_t c)
        : DenseMatrix(flat, static_cast<int>(r), static_cast<int>(c)) {}

    // Conversion to Eigen for test/interop compatibility
    operator Eigen::MatrixXd() const { return data; }

    int rows() const override { return static_cast<int>(data.rows()); }
    int cols() const override { return static_cast<int>(data.cols()); }

    double& at(std::size_t i, std::size_t j) { return data(i, j); }
    const double& at(std::size_t i, std::size_t j) const { return data(i, j); }

    // Eigen-style operator() for test/interop compatibility
    double& operator()(int i, int j) { return data(i, j); }
    const double& operator()(int i, int j) const { return data(i, j); }

    static DenseMatrix Zero(int r, int c) {
        DenseMatrix m(r, c);
        m.data.setZero();
        return m;
    }

    static DenseMatrix Identity(int n) {
        DenseMatrix m(n, n);
        m.data.setIdentity();
        return m;
    }

    static DenseMatrix Identity(std::size_t n) {
        return Identity(static_cast<int>(n));
    }

    std::unique_ptr<mytrix::MatrixBase> multiply(const DenseMatrix& other) const;
    std::unique_ptr<mytrix::MatrixBase> multiply(const DenseMatrix& other, const mytrix::OperationOptions& options) const;
    std::unique_ptr<mytrix::MatrixBase> transpose() const;
    std::unique_ptr<mytrix::MatrixBase> transpose(const mytrix::OperationOptions& options) const;
    std::unique_ptr<mytrix::MatrixBase> add(const DenseMatrix& other) const;
    std::unique_ptr<mytrix::MatrixBase> add(const DenseMatrix& other, const mytrix::OperationOptions& options) const;

    DenseMatrix operator+(const DenseMatrix& other) const {
        DenseMatrix result(rows(), cols());
        result.data = data + other.data;
        return result;
    }

    DenseMatrix operator-(const DenseMatrix& other) const {
        DenseMatrix result(rows(), cols());
        result.data = data - other.data;
        return result;
    }

    DenseMatrix operator*(const DenseMatrix& other) const {
        DenseMatrix result(rows(), other.cols());
        result.data = data * other.data;
        return result;
    }

    // Norm of the matrix (Frobenius norm)
    double norm() const { return data.norm(); }

    // Eigen array proxy for element-wise operations (e.g. .array().isFinite().all())
    auto array() const { return data.array(); }
    auto array() { return data.array(); }

    // Extract a row as a DenseVector
    mytrix::DenseVector row(int i) const;

    // Broadcast vector addition across rows: mat.rowwise() + vec.transpose()
    mytrix::DenseMatrix rowwise_add(const mytrix::DenseVector& vec) const;
};

class DenseVector {
public:
    Eigen::VectorXd data;

    DenseVector() : data(0) {}
    DenseVector(int n) : data(n) { data.setZero(); }
    DenseVector(const Eigen::VectorXd& v) : data(v) {}
    DenseVector(Eigen::VectorXd&& v) : data(std::move(v)) {}
    DenseVector(const std::vector<double>& flat, int n) : data(n) {
        for (int i = 0; i < n; ++i)
            data(i) = flat[static_cast<std::size_t>(i)];
    }
    DenseVector(const std::vector<double>& flat, std::size_t n)
        : DenseVector(flat, static_cast<int>(n)) {}

    operator Eigen::VectorXd() const { return data; }

    int size() const { return static_cast<int>(data.size()); }

    double& at(std::size_t i) { return data(i); }
    const double& at(std::size_t i) const { return data(i); }

    double& operator()(int i) { return data(i); }
    const double& operator()(int i) const { return data(i); }

    static DenseVector Zero(int n) {
        DenseVector v(n);
        v.data.setZero();
        return v;
    }

    static DenseVector Ones(int n) {
        DenseVector v(n);
        v.data.setOnes();
        return v;
    }

    static DenseVector Random(int n) {
        DenseVector v(n);
        v.data.setRandom();
        return v;
    }

    double norm() const { return data.norm(); }

    auto array() const { return data.array(); }
    auto array() { return data.array(); }

    // Return as a row vector (1 x n matrix)
    mytrix::DenseMatrix transpose() const;
};

// ...existing DenseVector class definition...

// Implementations must be after both classes are defined
inline mytrix::DenseMatrix mytrix::DenseVector::transpose() const {
    mytrix::DenseMatrix m(1, size());
    for (int i = 0; i < size(); ++i) m.data(0, i) = data(i);
    return m;
}

inline std::unique_ptr<mytrix::MatrixBase> mytrix::DenseMatrix::multiply(const mytrix::DenseMatrix& other) const {
    return multiply(other, mytrix::detail::default_operation_options());
}

inline std::unique_ptr<mytrix::MatrixBase> mytrix::DenseMatrix::multiply(const mytrix::DenseMatrix& other, const mytrix::OperationOptions& options) const {
    if (cols() != other.rows()) {
        throw std::invalid_argument("multiply: matrix dimension mismatch");
    }

    [[maybe_unused]] const mytrix::ComputeBackend backend = mytrix::detail::resolve_operation_backend(options);
    auto result = std::make_unique<mytrix::DenseMatrix>(rows(), other.cols());

    // Template-based compile-time dispatch to platform-specific backends
#ifdef MYTRIX_ENABLE_METAL
    if (backend == mytrix::ComputeBackend::GPU_METAL) {
        DenseMatrix temp = mytrix::metal::MetalBackend::multiply(*this, other, options.boost);
        result->data = temp.data;
        return result;
    }
#endif

#ifdef MYTRIX_ENABLE_CUDA
    if (backend == mytrix::ComputeBackend::GPU_CUDA) {
        DenseMatrix temp = mytrix::cuda::CudaBackend::multiply(*this, other);
        result->data = temp.data;
        return result;
    }
#endif

#ifdef MYTRIX_ENABLE_OPENCL
    if (backend == mytrix::ComputeBackend::GPU_OPENCL) {
        DenseMatrix temp = mytrix::opencl::OpenCLBackend::multiply(*this, other);
        result->data = temp.data;
        return result;
    }
#endif

    // CPU fallback (default path, always available)
    result->data = data * other.data;
    return result;
}

inline std::unique_ptr<mytrix::MatrixBase> mytrix::DenseMatrix::transpose() const {
    return transpose(mytrix::detail::default_operation_options());
}

inline std::unique_ptr<mytrix::MatrixBase> mytrix::DenseMatrix::transpose(const mytrix::OperationOptions& options) const {
    [[maybe_unused]] const mytrix::ComputeBackend backend = mytrix::detail::resolve_operation_backend(options);
    auto result = std::make_unique<mytrix::DenseMatrix>(cols(), rows());

    // Template-based compile-time dispatch to platform-specific backends
#ifdef MYTRIX_ENABLE_METAL
    if (backend == mytrix::ComputeBackend::GPU_METAL) {
        DenseMatrix temp = mytrix::metal::MetalBackend::transpose(*this, options.boost);
        result->data = temp.data;
        return result;
    }
#endif

#ifdef MYTRIX_ENABLE_CUDA
    if (backend == mytrix::ComputeBackend::GPU_CUDA) {
        DenseMatrix temp = mytrix::cuda::CudaBackend::transpose(*this);
        result->data = temp.data;
        return result;
    }
#endif

#ifdef MYTRIX_ENABLE_OPENCL
    if (backend == mytrix::ComputeBackend::GPU_OPENCL) {
        DenseMatrix temp = mytrix::opencl::OpenCLBackend::transpose(*this);
        result->data = temp.data;
        return result;
    }
#endif

    // CPU fallback (default path, always available)
    result->data = data.transpose();
    return result;
}

inline std::unique_ptr<mytrix::MatrixBase> mytrix::DenseMatrix::add(const mytrix::DenseMatrix& other) const {
    return add(other, mytrix::detail::default_operation_options());
}

inline std::unique_ptr<mytrix::MatrixBase> mytrix::DenseMatrix::add(const mytrix::DenseMatrix& other, const mytrix::OperationOptions& options) const {
    if (rows() != other.rows() || cols() != other.cols()) {
        throw std::invalid_argument("add: matrix dimension mismatch");
    }

    [[maybe_unused]] const mytrix::ComputeBackend backend = mytrix::detail::resolve_operation_backend(options);
    auto result = std::make_unique<mytrix::DenseMatrix>(rows(), cols());

    // Template-based compile-time dispatch to platform-specific backends
#ifdef MYTRIX_ENABLE_METAL
    if (backend == mytrix::ComputeBackend::GPU_METAL) {
        DenseMatrix temp = mytrix::metal::MetalBackend::add(*this, other, options.boost);
        result->data = temp.data;
        return result;
    }
#endif

#ifdef MYTRIX_ENABLE_CUDA
    if (backend == mytrix::ComputeBackend::GPU_CUDA) {
        DenseMatrix temp = mytrix::cuda::CudaBackend::add(*this, other);
        result->data = temp.data;
        return result;
    }
#endif

#ifdef MYTRIX_ENABLE_OPENCL
    if (backend == mytrix::ComputeBackend::GPU_OPENCL) {
        DenseMatrix temp = mytrix::opencl::OpenCLBackend::add(*this, other);
        result->data = temp.data;
        return result;
    }
#endif

    // CPU fallback (default path, always available)
    result->data = data + other.data;
    return result;
}

inline mytrix::DenseVector mytrix::DenseMatrix::row(int i) const {
    return mytrix::DenseVector(data.row(i));
}

inline mytrix::DenseMatrix mytrix::DenseMatrix::rowwise_add(const mytrix::DenseVector& vec) const {
    if (vec.size() != cols()) throw std::invalid_argument("rowwise_add: vector size mismatch");
    mytrix::DenseMatrix result(rows(), cols());
    for (int i = 0; i < rows(); ++i)
        for (int j = 0; j < cols(); ++j)
            result.data(i, j) = data(i, j) + vec.data(j);
    return result;
}

using Matrix = mytrix::DenseMatrix;
// using Vector = mytrix::DenseVector; // Disabled to use class-based Vector

inline mytrix::DenseMatrix mytrix::DenseMatrix::Ones(int r, int c) {
    mytrix::DenseMatrix m(r, c);
    m.data.setOnes();
    return m;
}
inline mytrix::DenseMatrix mytrix::DenseMatrix::Ones(std::size_t r, std::size_t c) {
    return Ones(static_cast<int>(r), static_cast<int>(c));
}
inline mytrix::DenseMatrix mytrix::DenseMatrix::Random(int r, int c) {
    mytrix::DenseMatrix m(r, c);
    m.data.setRandom();
    return m;
}
inline mytrix::DenseMatrix mytrix::DenseMatrix::Random(std::size_t r, std::size_t c) {
    return Random(static_cast<int>(r), static_cast<int>(c));
}

} // namespace mytrix
