package coolbox

import (
	"reflect"
	"testing"
)

func TestDenseMatrixOperations(t *testing.T) {
	if err := SetMatrixBackend(BackendEigen); err != nil {
		t.Fatal(err)
	}
	defer func() {
		if err := SetMatrixBackend(BackendCPU); err != nil {
			t.Error(err)
		}
	}()

	left, err := DenseMatrixFromData(2, 2, []float64{1, 2, 3, 4})
	if err != nil {
		t.Fatal(err)
	}
	defer left.Close()
	right, err := DenseMatrixFromData(2, 2, []float64{5, 6, 7, 8})
	if err != nil {
		t.Fatal(err)
	}
	defer right.Close()

	product, err := left.Multiply(right, OptimizationScalar)
	if err != nil {
		t.Fatal(err)
	}
	defer product.Close()
	if values, err := product.Data(); err != nil || !reflect.DeepEqual(values, []float64{19, 22, 43, 50}) {
		t.Fatalf("product.Data() = %v, %v", values, err)
	}

	transposed, err := left.Transpose()
	if err != nil {
		t.Fatal(err)
	}
	defer transposed.Close()
	if values, err := transposed.Data(); err != nil || !reflect.DeepEqual(values, []float64{1, 3, 2, 4}) {
		t.Fatalf("transpose.Data() = %v, %v", values, err)
	}
}

func TestMatrixBackendSelection(t *testing.T) {
	defer func() {
		if err := SetMatrixBackend(BackendCPU); err != nil {
			t.Error(err)
		}
	}()
	if available, err := MatrixBackendAvailable(BackendEigen); err != nil || !available {
		t.Fatalf("MatrixBackendAvailable(Eigen) = %v, %v; want true", available, err)
	}
	if name, err := MatrixBackendName(BackendEigen); err != nil || name != "eigen" {
		t.Fatalf("MatrixBackendName(Eigen) = %q, %v; want eigen", name, err)
	}
	if err := SetMatrixBackend(BackendEigen); err != nil {
		t.Fatal(err)
	}
	if got := CurrentMatrixBackend(); got != BackendEigen {
		t.Fatalf("CurrentMatrixBackend() = %v; want Eigen", got)
	}
	matrix, err := DenseMatrixFromData(1, 1, []float64{2})
	if err != nil {
		t.Fatal(err)
	}
	defer matrix.Close()
	result, err := matrix.Multiply(matrix, OptimizationAuto)
	if err != nil {
		t.Fatal(err)
	}
	defer result.Close()
	if got := ActiveMatrixBackend(); got != BackendEigen {
		t.Fatalf("ActiveMatrixBackend() = %v; want Eigen", got)
	}
	if err := SetMatrixBackend(BackendCPU); err != nil {
		t.Fatal(err)
	}
	if available, err := MatrixBackendAvailable(BackendAuto); err != nil || !available {
		t.Fatalf("MatrixBackendAvailable(Auto) = %v, %v; want true", available, err)
	}
	if available, err := MatrixBackendAvailable(BackendOpenCL); err != nil || available {
		t.Fatalf("MatrixBackendAvailable(OpenCL) = %v, %v; want false", available, err)
	}
	if err := SetMatrixBackend(BackendOpenCL); err == nil {
		t.Fatal("SetMatrixBackend(OpenCL) succeeded without an implementation")
	}
}

func TestDenseVectorOperations(t *testing.T) {
	if err := SetMatrixBackend(BackendEigen); err != nil {
		t.Fatal(err)
	}
	defer func() {
		if err := SetMatrixBackend(BackendCPU); err != nil {
			t.Error(err)
		}
	}()

	left, err := DenseVectorFromData([]float64{1, 2, 3})
	if err != nil {
		t.Fatal(err)
	}
	defer left.Close()
	right, err := DenseVectorFromData([]float64{4, 5, 6})
	if err != nil {
		t.Fatal(err)
	}
	defer right.Close()

	dot, err := left.Dot(right, OptimizationScalar)
	if err != nil || dot != 32 {
		t.Fatalf("Dot() = %v, %v; want 32", dot, err)
	}
	sum, err := left.Add(right, OptimizationAuto)
	if err != nil {
		t.Fatal(err)
	}
	defer sum.Close()
	if values, err := sum.Data(); err != nil || !reflect.DeepEqual(values, []float64{5, 7, 9}) {
		t.Fatalf("Add().Data() = %v, %v", values, err)
	}
}
