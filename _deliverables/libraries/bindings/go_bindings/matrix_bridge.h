#ifndef COOLBOX_MATRIX_BRIDGE_H
#define COOLBOX_MATRIX_BRIDGE_H

#include <stddef.h>

#ifdef __cplusplus
#include <vector>
namespace coolbox::matrix {

enum class CpuOptimization {
    Auto,
    Scalar,
    Avx2Fma,
    Neon,
};

enum class MatrixBackend {
    Cpu,
    Eigen,
    Auto,
    OpenCl,
    Cuda,
    Metal,
    OpenBlas,
};

class DenseVector {
public:
    explicit DenseVector(size_t size);
    DenseVector(const double* values, size_t size);

    size_t size() const noexcept;
    double at(size_t index) const;
    void set(size_t index, double value);
    const double* data() const noexcept;
    double norm() const noexcept;
    DenseVector add(const DenseVector& other, CpuOptimization optimization) const;
    double dot(const DenseVector& other, CpuOptimization optimization) const;

private:
    std::vector<double> values_;
};

class DenseMatrix {
public:
    DenseMatrix(size_t rows, size_t columns);
    DenseMatrix(size_t rows, size_t columns, const double* values, size_t count);

    static DenseMatrix identity(size_t size);
    size_t rows() const noexcept;
    size_t columns() const noexcept;
    double at(size_t row, size_t column) const;
    void set(size_t row, size_t column, double value);
    const double* data() const noexcept;
    DenseMatrix add(const DenseMatrix& other, CpuOptimization optimization) const;
    DenseMatrix subtract(const DenseMatrix& other) const;
    DenseMatrix multiply(const DenseMatrix& other, CpuOptimization optimization) const;
    DenseMatrix transpose() const;
    DenseMatrix rowwise_add(const DenseVector& vector, CpuOptimization optimization) const;
    double norm() const noexcept;

private:
    size_t rows_;
    size_t columns_;
    std::vector<double> values_;
};

const char* cpu_optimization_name(CpuOptimization optimization) noexcept;
bool cpu_optimization_available(CpuOptimization optimization) noexcept;
void set_cpu_threads(size_t count);
size_t cpu_threads() noexcept;
void set_matrix_backend(MatrixBackend backend);
MatrixBackend matrix_backend() noexcept;
MatrixBackend active_matrix_backend() noexcept;
const char* matrix_backend_name(MatrixBackend backend) noexcept;
bool matrix_backend_available(MatrixBackend backend) noexcept;

} // namespace coolbox::matrix

#endif

#ifdef __cplusplus
extern "C" {
#endif
void* coolbox_matrix_new(size_t rows, size_t columns, const double* values,
                         size_t count, char* error, size_t error_capacity);
void* coolbox_matrix_identity(size_t size, char* error, size_t error_capacity);
void coolbox_matrix_free(void* matrix);
size_t coolbox_matrix_rows(const void* matrix);
size_t coolbox_matrix_columns(const void* matrix);
int coolbox_matrix_copy_data(const void* matrix, double* output, size_t capacity,
                             char* error, size_t error_capacity);
int coolbox_matrix_get(const void* matrix, size_t row, size_t column, double* value,
                       char* error, size_t error_capacity);
int coolbox_matrix_set(void* matrix, size_t row, size_t column, double value,
                       char* error, size_t error_capacity);
void* coolbox_matrix_binary(const void* lhs, const void* rhs, int operation,
                            int optimization, char* error, size_t error_capacity);
void* coolbox_matrix_transpose(const void* matrix, char* error, size_t error_capacity);
void* coolbox_matrix_rowwise_add(const void* matrix, const void* vector,
                                 int optimization, char* error, size_t error_capacity);
double coolbox_matrix_norm(const void* matrix);

void* coolbox_vector_new(const double* values, size_t size,
                         char* error, size_t error_capacity);
void coolbox_vector_free(void* vector);
size_t coolbox_vector_size(const void* vector);
int coolbox_vector_copy_data(const void* vector, double* output, size_t capacity,
                             char* error, size_t error_capacity);
int coolbox_vector_get(const void* vector, size_t index, double* value,
                       char* error, size_t error_capacity);
int coolbox_vector_set(void* vector, size_t index, double value,
                       char* error, size_t error_capacity);
double coolbox_vector_norm(const void* vector);
int coolbox_vector_dot(const void* lhs, const void* rhs, int optimization, double* result,
                       char* error, size_t error_capacity);
void* coolbox_vector_add(const void* lhs, const void* rhs, int optimization,
                         char* error, size_t error_capacity);
void* coolbox_vector_transpose(const void* vector, char* error, size_t error_capacity);
const char* coolbox_cpu_optimization_name(int optimization);
int coolbox_cpu_optimization_available(int optimization);
void coolbox_matrix_set_cpu_threads(size_t count);
size_t coolbox_matrix_cpu_threads(void);
int coolbox_matrix_set_backend(int backend, char* error, size_t error_capacity);
int coolbox_matrix_get_backend(void);
int coolbox_matrix_get_active_backend(void);
const char* coolbox_matrix_backend_name(int backend);
int coolbox_matrix_backend_available(int backend);
#ifdef __cplusplus
}
#endif

#endif
