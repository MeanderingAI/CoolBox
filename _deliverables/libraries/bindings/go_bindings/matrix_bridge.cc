#include "matrix_bridge.h"

#include "cpu_optimizations.h"
#include "matrix_dense.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <climits>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" void coolbox_matrix_set_cpu_threads(size_t count) {
    coolbox::matrix::set_cpu_threads(count);
}

extern "C" size_t coolbox_matrix_cpu_threads(void) {
    return coolbox::matrix::cpu_threads();
}

namespace {

using coolbox::matrix::CpuOptimization;
using coolbox::matrix::DenseMatrix;
using coolbox::matrix::DenseVector;
using coolbox::matrix::MatrixBackend;

size_t checked_matrix_size(size_t rows, size_t columns) {
    if (rows == 0 || columns == 0 || rows > std::numeric_limits<size_t>::max() / columns) {
        throw std::invalid_argument("matrix dimensions must be positive and not overflow");
    }
    return rows * columns;
}

void set_error(char* output, size_t capacity, const char* message) {
    if (output == nullptr || capacity == 0) return;
    const size_t count = std::min(capacity - 1, std::strlen(message));
    std::memcpy(output, message, count);
    output[count] = '\0';
}

template <typename Function>
int invoke(char* error, size_t capacity, Function&& function) {
    try {
        function();
        return 1;
    } catch (const std::exception& exception) {
        set_error(error, capacity, exception.what());
    } catch (...) {
        set_error(error, capacity, "unknown native exception");
    }
    return 0;
}

template <typename Result, typename Function>
void* invoke_pointer(char* error, size_t capacity, Function&& function) {
    try {
        return new Result(function());
    } catch (const std::exception& exception) {
        set_error(error, capacity, exception.what());
    } catch (...) {
        set_error(error, capacity, "unknown native exception");
    }
    return nullptr;
}

CpuOptimization optimization(int value) {
    switch (value) {
    case 0: return CpuOptimization::Auto;
    case 1: return CpuOptimization::Scalar;
    case 2: return CpuOptimization::Avx2Fma;
    case 3: return CpuOptimization::Neon;
    default: throw std::invalid_argument("unknown CPU optimization");
    }
}

MatrixBackend matrix_backend(int value) {
    switch (value) {
    case 0: return MatrixBackend::Cpu;
    case 1: return MatrixBackend::Eigen;
    case 2: return MatrixBackend::Auto;
    case 3: return MatrixBackend::OpenCl;
    case 4: return MatrixBackend::Cuda;
    case 5: return MatrixBackend::Metal;
    case 6: return MatrixBackend::OpenBlas;
    default: throw std::invalid_argument("unknown matrix backend");
    }
}

mytrix::ComputeBackend compute_backend(MatrixBackend backend) {
    switch (backend) {
    case MatrixBackend::Cpu: return mytrix::ComputeBackend::CPU;
    case MatrixBackend::Eigen: return mytrix::ComputeBackend::EIGEN;
    case MatrixBackend::Auto: return mytrix::ComputeBackend::GPU_AUTO;
    case MatrixBackend::OpenCl: return mytrix::ComputeBackend::GPU_OPENCL;
    case MatrixBackend::Cuda: return mytrix::ComputeBackend::GPU_CUDA;
    case MatrixBackend::Metal: return mytrix::ComputeBackend::GPU_METAL;
    case MatrixBackend::OpenBlas: return mytrix::ComputeBackend::OPENBLAS;
    default: throw std::invalid_argument("unknown matrix backend");
    }
}

MatrixBackend from_compute_backend(mytrix::ComputeBackend backend) {
    switch (backend) {
    case mytrix::ComputeBackend::CPU:
    case mytrix::ComputeBackend::BOOST: return MatrixBackend::Cpu;
    case mytrix::ComputeBackend::EIGEN: return MatrixBackend::Eigen;
    case mytrix::ComputeBackend::GPU_AUTO: return MatrixBackend::Auto;
    case mytrix::ComputeBackend::GPU_OPENCL: return MatrixBackend::OpenCl;
    case mytrix::ComputeBackend::GPU_CUDA: return MatrixBackend::Cuda;
    case mytrix::ComputeBackend::GPU_METAL: return MatrixBackend::Metal;
    case mytrix::ComputeBackend::OPENBLAS: return MatrixBackend::OpenBlas;
    default: return MatrixBackend::Cpu;
    }
}

mytrix::ComputeBackend effective_backend(bool supports_openblas = false) {
    auto backend = mytrix::BackendConfig::resolve_backend(
        mytrix::BackendConfig::requested_backend(),
        mytrix::BackendConfig::boost_enabled());
    if (backend == mytrix::ComputeBackend::OPENBLAS && !supports_openblas)
        backend = mytrix::ComputeBackend::CPU;
    mytrix::BackendConfig::set_active_backend(backend);
    return backend;
}

bool use_gpu_backend(mytrix::ComputeBackend backend) noexcept {
    return backend == mytrix::ComputeBackend::GPU_CUDA ||
           backend == mytrix::ComputeBackend::GPU_METAL ||
           backend == mytrix::ComputeBackend::GPU_OPENCL;
}

int eigen_dimension(size_t value);
mytrix::DenseMatrix to_eigen_matrix(const DenseMatrix& matrix);
DenseMatrix from_eigen_matrix(const mytrix::DenseMatrix& matrix);

void require_host_vector_backend(mytrix::ComputeBackend backend) {
    if (use_gpu_backend(backend)) {
        throw std::runtime_error(
            "the selected GPU backend currently supports matrix add, multiply, and transpose only");
    }
}

DenseMatrix gpu_matrix_add(const DenseMatrix& lhs, const DenseMatrix& rhs,
                           mytrix::ComputeBackend backend) {
    const auto left = to_eigen_matrix(lhs);
    const auto right = to_eigen_matrix(rhs);
#if defined(MYTRIX_ENABLE_CUDA) && defined(__CUDACC__)
    if (backend == mytrix::ComputeBackend::GPU_CUDA) {
        return from_eigen_matrix(mytrix::cuda::CudaBackend::add_kernel(left, right));
    }
#endif
#if defined(MYTRIX_ENABLE_METAL) && defined(__APPLE__) && defined(__OBJC__)
    if (backend == mytrix::ComputeBackend::GPU_METAL) {
        return from_eigen_matrix(mytrix::metal::MetalBackend::add_kernel(left, right));
    }
#endif
    throw std::runtime_error("selected GPU backend is unavailable in this native build");
}

DenseMatrix gpu_matrix_multiply(const DenseMatrix& lhs, const DenseMatrix& rhs,
                                mytrix::ComputeBackend backend) {
    const auto left = to_eigen_matrix(lhs);
    const auto right = to_eigen_matrix(rhs);
#if defined(MYTRIX_ENABLE_CUDA) && defined(__CUDACC__)
    if (backend == mytrix::ComputeBackend::GPU_CUDA) {
        return from_eigen_matrix(mytrix::cuda::CudaBackend::multiply_kernel(left, right));
    }
#endif
#if defined(MYTRIX_ENABLE_METAL) && defined(__APPLE__) && defined(__OBJC__)
    if (backend == mytrix::ComputeBackend::GPU_METAL) {
        return from_eigen_matrix(mytrix::metal::MetalBackend::multiply_kernel(left, right));
    }
#endif
    throw std::runtime_error("selected GPU backend is unavailable in this native build");
}

DenseMatrix gpu_matrix_transpose(const DenseMatrix& matrix,
                                 mytrix::ComputeBackend backend) {
    const auto value = to_eigen_matrix(matrix);
#if defined(MYTRIX_ENABLE_CUDA) && defined(__CUDACC__)
    if (backend == mytrix::ComputeBackend::GPU_CUDA) {
        return from_eigen_matrix(mytrix::cuda::CudaBackend::transpose_kernel(value));
    }
#endif
#if defined(MYTRIX_ENABLE_METAL) && defined(__APPLE__) && defined(__OBJC__)
    if (backend == mytrix::ComputeBackend::GPU_METAL) {
        return from_eigen_matrix(mytrix::metal::MetalBackend::transpose_kernel(value));
    }
#endif
    throw std::runtime_error("selected GPU backend is unavailable in this native build");
}

int eigen_dimension(size_t value) {
    if (value > static_cast<size_t>(INT_MAX)) {
        throw std::invalid_argument("matrix dimensions exceed Eigen's supported range");
    }
    return static_cast<int>(value);
}

mytrix::DenseMatrix to_eigen_matrix(const DenseMatrix& matrix) {
    const int rows = eigen_dimension(matrix.rows());
    const int columns = eigen_dimension(matrix.columns());
    std::vector<double> values(matrix.rows() * matrix.columns());
    for (size_t row = 0; row < matrix.rows(); ++row) {
        for (size_t column = 0; column < matrix.columns(); ++column) {
            values[row * matrix.columns() + column] = matrix.at(row, column);
        }
    }
    return mytrix::DenseMatrix(values, rows, columns);
}

DenseMatrix from_eigen_matrix(const mytrix::DenseMatrix& matrix) {
    std::vector<double> values(static_cast<size_t>(matrix.rows()) *
                               static_cast<size_t>(matrix.cols()));
    for (int row = 0; row < matrix.rows(); ++row) {
        for (int column = 0; column < matrix.cols(); ++column) {
            values[static_cast<size_t>(row) * static_cast<size_t>(matrix.cols()) +
                   static_cast<size_t>(column)] = matrix.data(row, column);
        }
    }
    return DenseMatrix(static_cast<size_t>(matrix.rows()),
                       static_cast<size_t>(matrix.cols()), values.data(), values.size());
}

Eigen::VectorXd to_eigen_vector(const DenseVector& vector) {
    const int size = eigen_dimension(vector.size());
    Eigen::VectorXd result(size);
    for (size_t index = 0; index < vector.size(); ++index) {
        result(static_cast<int>(index)) = vector.at(index);
    }
    return result;
}

DenseVector from_eigen_vector(const Eigen::VectorXd& vector) {
    std::vector<double> values(static_cast<size_t>(vector.size()));
    for (Eigen::Index index = 0; index < vector.size(); ++index) {
        values[static_cast<size_t>(index)] = vector(index);
    }
    return DenseVector(values.data(), values.size());
}

template <typename T>
T& checked(void* handle, const char* kind) {
    if (handle == nullptr) throw std::invalid_argument(std::string(kind) + " is null");
    return *static_cast<T*>(handle);
}

template <typename T>
const T& checked(const void* handle, const char* kind) {
    if (handle == nullptr) throw std::invalid_argument(std::string(kind) + " is null");
    return *static_cast<const T*>(handle);
}

} // namespace

namespace coolbox::matrix {

void set_cpu_threads(size_t count) { mytrix::set_cpu_threads(count); }
size_t cpu_threads() noexcept { return mytrix::cpu_threads(); }

DenseVector::DenseVector(size_t size) : values_(size, 0.0) {}

DenseVector::DenseVector(const double* values, size_t size) : values_(size) {
    if (size != 0 && values == nullptr) {
        throw std::invalid_argument("non-empty vector data is null");
    }
    if (size != 0) std::copy_n(values, size, values_.begin());
}

size_t DenseVector::size() const noexcept { return values_.size(); }
double DenseVector::at(size_t index) const { return values_.at(index); }
void DenseVector::set(size_t index, double value) { values_.at(index) = value; }
const double* DenseVector::data() const noexcept { return values_.data(); }

double DenseVector::norm() const noexcept {
    const auto backend = effective_backend();
    if (use_gpu_backend(backend)) {
        mytrix::BackendConfig::set_active_backend(mytrix::ComputeBackend::EIGEN);
    }
    if ((backend == mytrix::ComputeBackend::EIGEN ||
         use_gpu_backend(backend)) && !values_.empty()) {
        const Eigen::Map<const Eigen::VectorXd> vector(
            data(), static_cast<Eigen::Index>(size()));
        return vector.norm();
    }
    double sum = 0.0;
    for (const double value : values_) sum += value * value;
    return std::sqrt(sum);
}

DenseVector DenseVector::add(const DenseVector& other, CpuOptimization selected) const {
    if (size() != other.size()) throw std::invalid_argument("vector size mismatch");
    const auto backend = effective_backend();
    require_host_vector_backend(backend);
    if (backend == mytrix::ComputeBackend::EIGEN) {
        const Eigen::VectorXd result = to_eigen_vector(*this) + to_eigen_vector(other);
        return from_eigen_vector(result);
    }
    DenseVector result(size());
    if (size() != 0) {
        mytrix::optimized_add(data(), other.data(), result.values_.data(), size(),
                              static_cast<mytrix::CpuOptimization>(selected));
    }
    return result;
}

double DenseVector::dot(const DenseVector& other, CpuOptimization selected) const {
    if (size() != other.size()) throw std::invalid_argument("vector size mismatch");
    const auto backend = effective_backend();
    require_host_vector_backend(backend);
    if (backend == mytrix::ComputeBackend::EIGEN) {
        return to_eigen_vector(*this).dot(to_eigen_vector(other));
    }
    return mytrix::optimized_dot(data(), other.data(), size(),
                                 static_cast<mytrix::CpuOptimization>(selected));
}

DenseMatrix::DenseMatrix(size_t rows, size_t columns)
    : rows_(rows), columns_(columns), values_(checked_matrix_size(rows, columns), 0.0) {}

DenseMatrix::DenseMatrix(size_t rows, size_t columns, const double* values, size_t count)
    : DenseMatrix(rows, columns) {
    if (count != 0 && (count != values_.size() || values == nullptr)) {
        throw std::invalid_argument("matrix data length does not match dimensions");
    }
    if (count != 0) std::copy_n(values, count, values_.begin());
}

DenseMatrix DenseMatrix::identity(size_t size) {
    DenseMatrix result(size, size);
    for (size_t index = 0; index < size; ++index) result.values_[index * size + index] = 1.0;
    return result;
}

size_t DenseMatrix::rows() const noexcept { return rows_; }
size_t DenseMatrix::columns() const noexcept { return columns_; }
double DenseMatrix::at(size_t row, size_t column) const {
    if (row >= rows_ || column >= columns_) throw std::out_of_range("matrix index out of range");
    return values_[row * columns_ + column];
}
void DenseMatrix::set(size_t row, size_t column, double value) {
    if (row >= rows_ || column >= columns_) throw std::out_of_range("matrix index out of range");
    values_[row * columns_ + column] = value;
}
const double* DenseMatrix::data() const noexcept { return values_.data(); }

DenseMatrix DenseMatrix::add(const DenseMatrix& other, CpuOptimization selected) const {
    if (rows_ != other.rows_ || columns_ != other.columns_) {
        throw std::invalid_argument("matrix dimensions do not match for addition");
    }
    const auto backend = effective_backend();
    if (use_gpu_backend(backend)) return gpu_matrix_add(*this, other, backend);
    if (backend == mytrix::ComputeBackend::EIGEN) {
        const auto left = to_eigen_matrix(*this);
        const auto right = to_eigen_matrix(other);
        return from_eigen_matrix(mytrix::DenseMatrix((left.data + right.data).eval()));
    }
    DenseMatrix result(rows_, columns_);
    mytrix::optimized_add(data(), other.data(), result.values_.data(), values_.size(),
                          static_cast<mytrix::CpuOptimization>(selected));
    return result;
}

DenseMatrix DenseMatrix::subtract(const DenseMatrix& other) const {
    if (rows_ != other.rows_ || columns_ != other.columns_) {
        throw std::invalid_argument("matrix dimensions do not match for subtraction");
    }
    const auto backend = effective_backend();
    if (use_gpu_backend(backend)) {
        throw std::runtime_error("the selected GPU backend does not support matrix subtraction");
    }
    if (backend == mytrix::ComputeBackend::EIGEN) {
        const auto left = to_eigen_matrix(*this);
        const auto right = to_eigen_matrix(other);
        return from_eigen_matrix(mytrix::DenseMatrix((left.data - right.data).eval()));
    }
    DenseMatrix result(rows_, columns_);
    for (size_t index = 0; index < values_.size(); ++index) {
        result.values_[index] = values_[index] - other.values_[index];
    }
    return result;
}

DenseMatrix DenseMatrix::multiply(const DenseMatrix& other, CpuOptimization selected) const {
    if (columns_ != other.rows_) throw std::invalid_argument("matrix dimension mismatch for multiplication");
    const auto backend = effective_backend(true);
#ifdef MYTRIX_ENABLE_OPENBLAS
    if (backend == mytrix::ComputeBackend::OPENBLAS) {
        DenseMatrix result(rows_, other.columns_);
        mytrix::detail::openblas_multiply(data(), other.data(), result.values_.data(),
                                         rows_, columns_, other.columns_, true);
        return result;
    }
#endif
    if (use_gpu_backend(backend)) return gpu_matrix_multiply(*this, other, backend);
    if (backend == mytrix::ComputeBackend::EIGEN) {
        using RowMatrix = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
        const auto m = eigen_dimension(rows_), n = eigen_dimension(other.columns_), k = eigen_dimension(columns_);
        DenseMatrix result(rows_, other.columns_);
        const Eigen::Map<const RowMatrix> left(data(), m, k), right(other.data(), k, n);
        Eigen::Map<RowMatrix> output(result.values_.data(), m, n);
        mytrix::parallel_matrix_rows(rows_, other.columns_, columns_, [&](size_t first, size_t last) {
            const auto count = static_cast<Eigen::Index>(last - first);
            output.middleRows(static_cast<Eigen::Index>(first), count).noalias() =
                left.middleRows(static_cast<Eigen::Index>(first), count) * right;
        });
        return result;
    }
    DenseMatrix result(rows_, other.columns_);
    mytrix::optimized_multiply(data(), other.data(), result.values_.data(),
        rows_, columns_, other.columns_, static_cast<mytrix::CpuOptimization>(selected));
    return result;
}

DenseMatrix DenseMatrix::transpose() const {
    const auto backend = effective_backend();
    if (use_gpu_backend(backend)) return gpu_matrix_transpose(*this, backend);
    if (backend == mytrix::ComputeBackend::EIGEN) {
        const auto value = to_eigen_matrix(*this);
        return from_eigen_matrix(mytrix::DenseMatrix(value.data.transpose().eval()));
    }
    DenseMatrix result(columns_, rows_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < columns_; ++j) {
            result.values_[j * rows_ + i] = values_[i * columns_ + j];
        }
    }
    return result;
}

DenseMatrix DenseMatrix::rowwise_add(const DenseVector& vector, CpuOptimization selected) const {
    if (vector.size() != columns_) throw std::invalid_argument("row-wise vector size mismatch");
    const auto backend = effective_backend();
    if (use_gpu_backend(backend)) {
        throw std::runtime_error("the selected GPU backend does not support row-wise addition");
    }
    if (backend == mytrix::ComputeBackend::EIGEN) {
        const auto matrix = to_eigen_matrix(*this);
        const auto values = to_eigen_vector(vector);
        return from_eigen_matrix(mytrix::DenseMatrix(
            (matrix.data.rowwise() + values.transpose()).eval()));
    }
    DenseMatrix result(rows_, columns_);
    for (size_t i = 0; i < rows_; ++i) {
        mytrix::optimized_add(values_.data() + i * columns_, vector.data(),
                              result.values_.data() + i * columns_, columns_,
                              static_cast<mytrix::CpuOptimization>(selected));
    }
    return result;
}

double DenseMatrix::norm() const noexcept {
    const auto backend = mytrix::BackendConfig::resolve_backend(
        mytrix::BackendConfig::requested_backend(),
        mytrix::BackendConfig::boost_enabled());
    mytrix::BackendConfig::set_active_backend(
        use_gpu_backend(backend) ? mytrix::ComputeBackend::EIGEN : backend);
    if (backend == mytrix::ComputeBackend::EIGEN || use_gpu_backend(backend)) {
        using RowMajorMatrix =
            Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
        const Eigen::Map<const RowMajorMatrix> matrix(
            data(), static_cast<Eigen::Index>(rows_), static_cast<Eigen::Index>(columns_));
        return matrix.norm();
    }
    double sum = 0.0;
    for (const double value : values_) sum += value * value;
    return std::sqrt(sum);
}

const char* cpu_optimization_name(CpuOptimization value) noexcept {
    return mytrix::cpu_optimization_name(static_cast<mytrix::CpuOptimization>(value));
}

bool cpu_optimization_available(CpuOptimization value) noexcept {
    return mytrix::cpu_optimization_available(static_cast<mytrix::CpuOptimization>(value));
}

void set_matrix_backend(MatrixBackend backend) {
    mytrix::set_backend(compute_backend(backend));
}

MatrixBackend matrix_backend() noexcept {
    return from_compute_backend(mytrix::BackendConfig::requested_backend());
}

MatrixBackend active_matrix_backend() noexcept {
    return from_compute_backend(mytrix::BackendConfig::active_backend());
}

const char* matrix_backend_name(MatrixBackend backend) noexcept {
    switch (backend) {
    case MatrixBackend::Cpu: return "cpu";
    case MatrixBackend::Eigen: return "eigen";
    case MatrixBackend::Auto: return "auto";
    case MatrixBackend::OpenCl: return "opencl";
    case MatrixBackend::Cuda: return "cuda";
    case MatrixBackend::Metal: return "metal";
    case MatrixBackend::OpenBlas: return "openblas";
    default: return "unknown";
    }
}

bool matrix_backend_available(MatrixBackend backend) noexcept {
    if (backend == MatrixBackend::Auto) return true;
    try {
        return mytrix::BackendConfig::is_backend_available(compute_backend(backend));
    } catch (...) {
        return false;
    }
}

} // namespace coolbox::matrix

extern "C" {

void* coolbox_matrix_new(size_t rows, size_t columns, const double* values,
                         size_t count, char* error, size_t error_capacity) {
    return invoke_pointer<DenseMatrix>(error, error_capacity, [&] {
        return DenseMatrix(rows, columns, values, count);
    });
}

void* coolbox_matrix_identity(size_t size, char* error, size_t error_capacity) {
    return invoke_pointer<DenseMatrix>(error, error_capacity, [&] { return DenseMatrix::identity(size); });
}

void coolbox_matrix_free(void* matrix) { delete static_cast<DenseMatrix*>(matrix); }
size_t coolbox_matrix_rows(const void* matrix) { return matrix ? static_cast<const DenseMatrix*>(matrix)->rows() : 0; }
size_t coolbox_matrix_columns(const void* matrix) { return matrix ? static_cast<const DenseMatrix*>(matrix)->columns() : 0; }

int coolbox_matrix_copy_data(const void* handle, double* output, size_t capacity,
                             char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        const auto& matrix = checked<DenseMatrix>(handle, "matrix");
        const size_t count = matrix.rows() * matrix.columns();
        if (capacity < count || (count != 0 && output == nullptr)) {
            throw std::invalid_argument("matrix output buffer is too small");
        }
        std::copy_n(matrix.data(), count, output);
    });
}

int coolbox_matrix_get(const void* handle, size_t row, size_t column, double* value,
                       char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (value == nullptr) throw std::invalid_argument("matrix value output is null");
        *value = checked<DenseMatrix>(handle, "matrix").at(row, column);
    });
}

int coolbox_matrix_set(void* handle, size_t row, size_t column, double value,
                       char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { checked<DenseMatrix>(handle, "matrix").set(row, column, value); });
}

void* coolbox_matrix_binary(const void* lhs, const void* rhs, int operation,
                            int selected, char* error, size_t error_capacity) {
    return invoke_pointer<DenseMatrix>(error, error_capacity, [&] {
        const auto& left = checked<DenseMatrix>(lhs, "left matrix");
        const auto& right = checked<DenseMatrix>(rhs, "right matrix");
        switch (operation) {
        case 0: return left.add(right, optimization(selected));
        case 1: return left.subtract(right);
        case 2: return left.multiply(right, optimization(selected));
        default: throw std::invalid_argument("unknown matrix operation");
        }
    });
}

void* coolbox_matrix_transpose(const void* handle, char* error, size_t error_capacity) {
    return invoke_pointer<DenseMatrix>(error, error_capacity, [&] {
        return checked<DenseMatrix>(handle, "matrix").transpose();
    });
}

void* coolbox_matrix_rowwise_add(const void* matrix, const void* vector, int selected,
                                 char* error, size_t error_capacity) {
    return invoke_pointer<DenseMatrix>(error, error_capacity, [&] {
        return checked<DenseMatrix>(matrix, "matrix").rowwise_add(
            checked<DenseVector>(vector, "vector"), optimization(selected));
    });
}

double coolbox_matrix_norm(const void* handle) {
    return handle ? static_cast<const DenseMatrix*>(handle)->norm() : 0.0;
}

void* coolbox_vector_new(const double* values, size_t size, char* error, size_t error_capacity) {
    return invoke_pointer<DenseVector>(error, error_capacity, [&] { return DenseVector(values, size); });
}
void coolbox_vector_free(void* vector) { delete static_cast<DenseVector*>(vector); }
size_t coolbox_vector_size(const void* vector) { return vector ? static_cast<const DenseVector*>(vector)->size() : 0; }

int coolbox_vector_copy_data(const void* handle, double* output, size_t capacity,
                             char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        const auto& vector = checked<DenseVector>(handle, "vector");
        if (capacity < vector.size() || (vector.size() != 0 && output == nullptr)) {
            throw std::invalid_argument("vector output buffer is too small");
        }
        if (vector.size() != 0) std::copy_n(vector.data(), vector.size(), output);
    });
}

int coolbox_vector_get(const void* handle, size_t index, double* value,
                       char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (value == nullptr) throw std::invalid_argument("vector value output is null");
        *value = checked<DenseVector>(handle, "vector").at(index);
    });
}

int coolbox_vector_set(void* handle, size_t index, double value, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] { checked<DenseVector>(handle, "vector").set(index, value); });
}

double coolbox_vector_norm(const void* vector) {
    return vector ? static_cast<const DenseVector*>(vector)->norm() : 0.0;
}

int coolbox_vector_dot(const void* lhs, const void* rhs, int selected, double* result,
                       char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (result == nullptr) throw std::invalid_argument("dot-product output is null");
        *result = checked<DenseVector>(lhs, "left vector").dot(
            checked<DenseVector>(rhs, "right vector"), optimization(selected));
    });
}

void* coolbox_vector_add(const void* lhs, const void* rhs, int selected,
                         char* error, size_t error_capacity) {
    return invoke_pointer<DenseVector>(error, error_capacity, [&] {
        return checked<DenseVector>(lhs, "left vector").add(
            checked<DenseVector>(rhs, "right vector"), optimization(selected));
    });
}

void* coolbox_vector_transpose(const void* vector, char* error, size_t error_capacity) {
    return invoke_pointer<DenseMatrix>(error, error_capacity, [&] {
        const auto& values = checked<DenseVector>(vector, "vector");
        DenseMatrix result(1, values.size());
        for (size_t i = 0; i < values.size(); ++i) result.set(0, i, values.at(i));
        return result;
    });
}

const char* coolbox_cpu_optimization_name(int value) {
    try { return coolbox::matrix::cpu_optimization_name(optimization(value)); }
    catch (...) { return "unknown"; }
}

int coolbox_cpu_optimization_available(int value) {
    try { return coolbox::matrix::cpu_optimization_available(optimization(value)); }
    catch (...) { return 0; }
}

int coolbox_matrix_set_backend(int value, char* error, size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        coolbox::matrix::set_matrix_backend(matrix_backend(value));
    });
}

int coolbox_matrix_get_backend(void) {
    return static_cast<int>(coolbox::matrix::matrix_backend());
}

int coolbox_matrix_get_active_backend(void) {
    return static_cast<int>(coolbox::matrix::active_matrix_backend());
}

const char* coolbox_matrix_backend_name(int value) {
    try { return coolbox::matrix::matrix_backend_name(matrix_backend(value)); }
    catch (...) { return "unknown"; }
}

int coolbox_matrix_backend_available(int value) {
    try { return coolbox::matrix::matrix_backend_available(matrix_backend(value)); }
    catch (...) { return 0; }
}

} // extern "C"
