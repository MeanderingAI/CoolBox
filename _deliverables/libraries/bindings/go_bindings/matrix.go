package coolbox

/*
#cgo coolbox_openblas CXXFLAGS: -DMYTRIX_ENABLE_OPENBLAS=1
#cgo coolbox_openblas LDFLAGS: -lopenblas
#include "matrix_bridge.h"
*/
import "C"

import (
	"errors"
	"fmt"
	"unsafe"
)

// CpuOptimization selects a CPU strategy used when the CPU backend is active.
type CpuOptimization int

const (
	OptimizationAuto CpuOptimization = iota
	OptimizationScalar
	OptimizationAvx2Fma
	OptimizationNeon
)

// MatrixBackend selects the implementation used by dense matrix/vector operations.
type MatrixBackend int

// SetMatrixCPUThreads sets the CPU multiplication thread budget (0 = automatic).
func SetMatrixCPUThreads(count int) error {
	if count < 0 {
		return errors.New("coolbox: CPU thread count must not be negative")
	}
	C.coolbox_matrix_set_cpu_threads(C.size_t(count))
	return nil
}

// MatrixCPUThreads returns the resolved CPU thread budget; small products stay serial.
func MatrixCPUThreads() int {
	return int(C.coolbox_matrix_cpu_threads())
}

const (
	BackendCPU MatrixBackend = iota
	BackendEigen
	BackendAuto
	BackendOpenCL
	BackendCUDA
	BackendMetal
	BackendOpenBLAS
)

func (backend MatrixBackend) validate() error {
	if backend < BackendCPU || backend > BackendOpenBLAS {
		return fmt.Errorf("coolbox: unknown matrix backend %d", backend)
	}
	return nil
}

// SetMatrixBackend changes the process-wide matrix backend.
func SetMatrixBackend(backend MatrixBackend) error {
	if err := backend.validate(); err != nil {
		return err
	}
	var errorBuffer [nativeErrorCapacity]C.char
	if C.coolbox_matrix_set_backend(C.int(backend), &errorBuffer[0], nativeErrorCapacity) == 0 {
		return matrixError(&errorBuffer, "coolbox: failed to set matrix backend")
	}
	return nil
}

// CurrentMatrixBackend returns the requested process-wide matrix backend.
func CurrentMatrixBackend() MatrixBackend {
	return MatrixBackend(C.coolbox_matrix_get_backend())
}

// ActiveMatrixBackend returns the backend resolved for the most recent operation.
func ActiveMatrixBackend() MatrixBackend {
	return MatrixBackend(C.coolbox_matrix_get_active_backend())
}

// MatrixBackendName returns the native name of a backend.
func MatrixBackendName(backend MatrixBackend) (string, error) {
	if err := backend.validate(); err != nil {
		return "", err
	}
	return C.GoString(C.coolbox_matrix_backend_name(C.int(backend))), nil
}

// MatrixBackendAvailable reports whether a backend is available in this build.
func MatrixBackendAvailable(backend MatrixBackend) (bool, error) {
	if err := backend.validate(); err != nil {
		return false, err
	}
	return C.coolbox_matrix_backend_available(C.int(backend)) != 0, nil
}

func (optimization CpuOptimization) validate() error {
	if optimization < OptimizationAuto || optimization > OptimizationNeon {
		return fmt.Errorf("coolbox: unknown CPU optimization %d", optimization)
	}
	return nil
}

// CpuOptimizationName returns the native name for an optimization strategy.
func CpuOptimizationName(optimization CpuOptimization) (string, error) {
	if err := optimization.validate(); err != nil {
		return "", err
	}
	return C.GoString(C.coolbox_cpu_optimization_name(C.int(optimization))), nil
}

// CpuOptimizationAvailable reports whether the requested strategy is available.
func CpuOptimizationAvailable(optimization CpuOptimization) (bool, error) {
	if err := optimization.validate(); err != nil {
		return false, err
	}
	return C.coolbox_cpu_optimization_available(C.int(optimization)) != 0, nil
}

func matrixError(buffer *[nativeErrorCapacity]C.char, fallback string) error {
	message := C.GoString(&buffer[0])
	if message == "" {
		message = fallback
	}
	return errors.New(message)
}

func nativeDoublePointer(values []float64) *C.double {
	if len(values) == 0 {
		return nil
	}
	return (*C.double)(unsafe.Pointer(&values[0]))
}

func matrixElementCount(rows, columns int) (int, error) {
	maxInt := int(^uint(0) >> 1)
	if rows <= 0 || columns <= 0 || rows > maxInt/columns {
		return 0, errors.New("coolbox: matrix dimensions must be positive and not overflow")
	}
	return rows * columns, nil
}

// DenseMatrix is a row-major dense matrix backed by the CoolBox MATRIX library.
type DenseMatrix struct {
	handle unsafe.Pointer
}

// NewDenseMatrix allocates a zero-filled matrix.
func NewDenseMatrix(rows, columns int) (*DenseMatrix, error) {
	count, err := matrixElementCount(rows, columns)
	if err != nil {
		return nil, err
	}
	return newDenseMatrix(rows, columns, make([]float64, count))
}

// DenseMatrixZeros allocates a zero-filled matrix.
func DenseMatrixZeros(rows, columns int) (*DenseMatrix, error) {
	return NewDenseMatrix(rows, columns)
}

// DenseMatrixOnes allocates a matrix filled with ones.
func DenseMatrixOnes(rows, columns int) (*DenseMatrix, error) {
	count, err := matrixElementCount(rows, columns)
	if err != nil {
		return nil, err
	}
	values := make([]float64, count)
	for index := range values {
		values[index] = 1
	}
	return newDenseMatrix(rows, columns, values)
}

// DenseMatrixFromData creates a matrix from row-major values.
func DenseMatrixFromData(rows, columns int, values []float64) (*DenseMatrix, error) {
	if _, err := matrixElementCount(rows, columns); err != nil {
		return nil, err
	}
	if len(values) != rows*columns {
		return nil, errors.New("coolbox: matrix data length does not match dimensions")
	}
	return newDenseMatrix(rows, columns, values)
}

func newDenseMatrix(rows, columns int, values []float64) (*DenseMatrix, error) {
	var errorBuffer [nativeErrorCapacity]C.char
	handle := C.coolbox_matrix_new(C.size_t(rows), C.size_t(columns), nativeDoublePointer(values),
		C.size_t(len(values)), &errorBuffer[0], nativeErrorCapacity)
	if handle == nil {
		return nil, matrixError(&errorBuffer, "coolbox: failed to create matrix")
	}
	return &DenseMatrix{handle: handle}, nil
}

// DenseMatrixIdentity creates an identity matrix.
func DenseMatrixIdentity(size int) (*DenseMatrix, error) {
	if size <= 0 {
		return nil, errors.New("coolbox: identity size must be positive")
	}
	var errorBuffer [nativeErrorCapacity]C.char
	handle := C.coolbox_matrix_identity(C.size_t(size), &errorBuffer[0], nativeErrorCapacity)
	if handle == nil {
		return nil, matrixError(&errorBuffer, "coolbox: failed to create identity matrix")
	}
	return &DenseMatrix{handle: handle}, nil
}

func (matrix *DenseMatrix) nativeHandle() (unsafe.Pointer, error) {
	if matrix == nil || matrix.handle == nil {
		return nil, errors.New("coolbox: matrix is closed")
	}
	return matrix.handle, nil
}

// Close releases the native matrix.
func (matrix *DenseMatrix) Close() error {
	if matrix != nil && matrix.handle != nil {
		C.coolbox_matrix_free(matrix.handle)
		matrix.handle = nil
	}
	return nil
}

// Rows returns the row count.
func (matrix *DenseMatrix) Rows() (int, error) {
	handle, err := matrix.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_matrix_rows(handle)), nil
}

// Columns returns the column count.
func (matrix *DenseMatrix) Columns() (int, error) {
	handle, err := matrix.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_matrix_columns(handle)), nil
}

// Data returns a copy of the matrix's row-major values.
func (matrix *DenseMatrix) Data() ([]float64, error) {
	rows, err := matrix.Rows()
	if err != nil {
		return nil, err
	}
	columns, err := matrix.Columns()
	if err != nil {
		return nil, err
	}
	values := make([]float64, rows*columns)
	handle, _ := matrix.nativeHandle()
	var errorBuffer [nativeErrorCapacity]C.char
	if C.coolbox_matrix_copy_data(handle, nativeDoublePointer(values), C.size_t(len(values)),
		&errorBuffer[0], nativeErrorCapacity) == 0 {
		return nil, matrixError(&errorBuffer, "coolbox: failed to read matrix data")
	}
	return values, nil
}

// At returns one matrix element.
func (matrix *DenseMatrix) At(row, column int) (float64, error) {
	handle, err := matrix.nativeHandle()
	if err != nil {
		return 0, err
	}
	if row < 0 || column < 0 {
		return 0, errors.New("coolbox: matrix index out of range")
	}
	var value C.double
	var errorBuffer [nativeErrorCapacity]C.char
	if C.coolbox_matrix_get(handle, C.size_t(row), C.size_t(column), &value,
		&errorBuffer[0], nativeErrorCapacity) == 0 {
		return 0, matrixError(&errorBuffer, "coolbox: failed to read matrix element")
	}
	return float64(value), nil
}

// Set updates one matrix element.
func (matrix *DenseMatrix) Set(row, column int, value float64) error {
	handle, err := matrix.nativeHandle()
	if err != nil {
		return err
	}
	if row < 0 || column < 0 {
		return errors.New("coolbox: matrix index out of range")
	}
	var errorBuffer [nativeErrorCapacity]C.char
	if C.coolbox_matrix_set(handle, C.size_t(row), C.size_t(column), C.double(value),
		&errorBuffer[0], nativeErrorCapacity) == 0 {
		return matrixError(&errorBuffer, "coolbox: failed to set matrix element")
	}
	return nil
}

func (matrix *DenseMatrix) binary(other *DenseMatrix, operation int, optimization CpuOptimization) (*DenseMatrix, error) {
	if err := optimization.validate(); err != nil {
		return nil, err
	}
	left, err := matrix.nativeHandle()
	if err != nil {
		return nil, err
	}
	right, err := other.nativeHandle()
	if err != nil {
		return nil, err
	}
	var errorBuffer [nativeErrorCapacity]C.char
	handle := C.coolbox_matrix_binary(left, right, C.int(operation), C.int(optimization),
		&errorBuffer[0], nativeErrorCapacity)
	if handle == nil {
		return nil, matrixError(&errorBuffer, "coolbox: matrix operation failed")
	}
	return &DenseMatrix{handle: handle}, nil
}

// Add returns the element-wise sum.
func (matrix *DenseMatrix) Add(other *DenseMatrix, optimization CpuOptimization) (*DenseMatrix, error) {
	return matrix.binary(other, 0, optimization)
}

// Subtract returns the element-wise difference.
func (matrix *DenseMatrix) Subtract(other *DenseMatrix) (*DenseMatrix, error) {
	return matrix.binary(other, 1, OptimizationAuto)
}

// Multiply returns the matrix product.
func (matrix *DenseMatrix) Multiply(other *DenseMatrix, optimization CpuOptimization) (*DenseMatrix, error) {
	return matrix.binary(other, 2, optimization)
}

// Transpose returns a transposed copy.
func (matrix *DenseMatrix) Transpose() (*DenseMatrix, error) {
	handle, err := matrix.nativeHandle()
	if err != nil {
		return nil, err
	}
	var errorBuffer [nativeErrorCapacity]C.char
	result := C.coolbox_matrix_transpose(handle, &errorBuffer[0], nativeErrorCapacity)
	if result == nil {
		return nil, matrixError(&errorBuffer, "coolbox: matrix transpose failed")
	}
	return &DenseMatrix{handle: result}, nil
}

// RowwiseAdd broadcasts a vector across each matrix row.
func (matrix *DenseMatrix) RowwiseAdd(vector *DenseVector, optimization CpuOptimization) (*DenseMatrix, error) {
	if err := optimization.validate(); err != nil {
		return nil, err
	}
	matrixHandle, err := matrix.nativeHandle()
	if err != nil {
		return nil, err
	}
	vectorHandle, err := vector.nativeHandle()
	if err != nil {
		return nil, err
	}
	var errorBuffer [nativeErrorCapacity]C.char
	result := C.coolbox_matrix_rowwise_add(matrixHandle, vectorHandle, C.int(optimization),
		&errorBuffer[0], nativeErrorCapacity)
	if result == nil {
		return nil, matrixError(&errorBuffer, "coolbox: row-wise addition failed")
	}
	return &DenseMatrix{handle: result}, nil
}

// Norm returns the Frobenius norm.
func (matrix *DenseMatrix) Norm() (float64, error) {
	handle, err := matrix.nativeHandle()
	if err != nil {
		return 0, err
	}
	return float64(C.coolbox_matrix_norm(handle)), nil
}

// DenseVector is a vector backed by the CoolBox MATRIX library.
type DenseVector struct {
	handle unsafe.Pointer
}

// NewDenseVector constructs a zero-filled vector.
func NewDenseVector(size int) (*DenseVector, error) {
	if size < 0 {
		return nil, errors.New("coolbox: vector size must not be negative")
	}
	return DenseVectorFromData(make([]float64, size))
}

// DenseVectorFromData creates a vector from values.
func DenseVectorFromData(values []float64) (*DenseVector, error) {
	var errorBuffer [nativeErrorCapacity]C.char
	handle := C.coolbox_vector_new(nativeDoublePointer(values), C.size_t(len(values)),
		&errorBuffer[0], nativeErrorCapacity)
	if handle == nil {
		return nil, matrixError(&errorBuffer, "coolbox: failed to create vector")
	}
	return &DenseVector{handle: handle}, nil
}

func (vector *DenseVector) nativeHandle() (unsafe.Pointer, error) {
	if vector == nil || vector.handle == nil {
		return nil, errors.New("coolbox: vector is closed")
	}
	return vector.handle, nil
}

// Close releases the native vector.
func (vector *DenseVector) Close() error {
	if vector != nil && vector.handle != nil {
		C.coolbox_vector_free(vector.handle)
		vector.handle = nil
	}
	return nil
}

// Size returns the number of vector elements.
func (vector *DenseVector) Size() (int, error) {
	handle, err := vector.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_vector_size(handle)), nil
}

// Data returns a copy of the vector values.
func (vector *DenseVector) Data() ([]float64, error) {
	size, err := vector.Size()
	if err != nil {
		return nil, err
	}
	values := make([]float64, size)
	handle, _ := vector.nativeHandle()
	var errorBuffer [nativeErrorCapacity]C.char
	if C.coolbox_vector_copy_data(handle, nativeDoublePointer(values), C.size_t(size),
		&errorBuffer[0], nativeErrorCapacity) == 0 {
		return nil, matrixError(&errorBuffer, "coolbox: failed to read vector data")
	}
	return values, nil
}

// At returns an element.
func (vector *DenseVector) At(index int) (float64, error) {
	handle, err := vector.nativeHandle()
	if err != nil {
		return 0, err
	}
	if index < 0 {
		return 0, errors.New("coolbox: vector index out of range")
	}
	var value C.double
	var errorBuffer [nativeErrorCapacity]C.char
	if C.coolbox_vector_get(handle, C.size_t(index), &value, &errorBuffer[0], nativeErrorCapacity) == 0 {
		return 0, matrixError(&errorBuffer, "coolbox: failed to read vector element")
	}
	return float64(value), nil
}

// Set updates an element.
func (vector *DenseVector) Set(index int, value float64) error {
	handle, err := vector.nativeHandle()
	if err != nil {
		return err
	}
	if index < 0 {
		return errors.New("coolbox: vector index out of range")
	}
	var errorBuffer [nativeErrorCapacity]C.char
	if C.coolbox_vector_set(handle, C.size_t(index), C.double(value),
		&errorBuffer[0], nativeErrorCapacity) == 0 {
		return matrixError(&errorBuffer, "coolbox: failed to set vector element")
	}
	return nil
}

// Norm returns the Euclidean norm.
func (vector *DenseVector) Norm() (float64, error) {
	handle, err := vector.nativeHandle()
	if err != nil {
		return 0, err
	}
	return float64(C.coolbox_vector_norm(handle)), nil
}

// Dot computes the dot product.
func (vector *DenseVector) Dot(other *DenseVector, optimization CpuOptimization) (float64, error) {
	if err := optimization.validate(); err != nil {
		return 0, err
	}
	left, err := vector.nativeHandle()
	if err != nil {
		return 0, err
	}
	right, err := other.nativeHandle()
	if err != nil {
		return 0, err
	}
	var errorBuffer [nativeErrorCapacity]C.char
	var result C.double
	if C.coolbox_vector_dot(left, right, C.int(optimization), &result, &errorBuffer[0], nativeErrorCapacity) == 0 {
		return 0, matrixError(&errorBuffer, "coolbox: vector dot product failed")
	}
	return float64(result), nil
}

// Add returns the element-wise sum.
func (vector *DenseVector) Add(other *DenseVector, optimization CpuOptimization) (*DenseVector, error) {
	if err := optimization.validate(); err != nil {
		return nil, err
	}
	left, err := vector.nativeHandle()
	if err != nil {
		return nil, err
	}
	right, err := other.nativeHandle()
	if err != nil {
		return nil, err
	}
	var errorBuffer [nativeErrorCapacity]C.char
	result := C.coolbox_vector_add(left, right, C.int(optimization), &errorBuffer[0], nativeErrorCapacity)
	if result == nil {
		return nil, matrixError(&errorBuffer, "coolbox: vector addition failed")
	}
	return &DenseVector{handle: result}, nil
}

// Transpose returns the vector as a one-row matrix.
func (vector *DenseVector) Transpose() (*DenseMatrix, error) {
	handle, err := vector.nativeHandle()
	if err != nil {
		return nil, err
	}
	var errorBuffer [nativeErrorCapacity]C.char
	result := C.coolbox_vector_transpose(handle, &errorBuffer[0], nativeErrorCapacity)
	if result == nil {
		return nil, matrixError(&errorBuffer, "coolbox: vector transpose failed")
	}
	return &DenseMatrix{handle: result}, nil
}
