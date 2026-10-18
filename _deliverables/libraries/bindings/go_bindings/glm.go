package coolbox

/*
#include <stdlib.h>
void* coolbox_glm_linear_regression_new(unsigned int iterations, double learning_rate, int fit_type);
void coolbox_glm_linear_regression_free(void* model);
int coolbox_glm_linear_regression_fit(void* model, const double* x, size_t rows, size_t cols, const double* y, char* error, size_t error_capacity);
int coolbox_glm_linear_regression_predict(void* model, const double* sample, size_t cols, double* prediction, char* error, size_t error_capacity);
int coolbox_glm_linear_regression_coefficients(void* model, double* weights, size_t weight_capacity, double* intercept, char* error, size_t error_capacity);
size_t coolbox_glm_linear_regression_feature_count(void* model);
*/
import "C"

import (
	"errors"
	"fmt"
	"math"
	"unsafe"
)

const nativeErrorCapacity = 512

// LinearRegressionType selects the algorithm used to fit linear regression.
type LinearRegressionType int

const (
	GradientDescent LinearRegressionType = iota
	ClosedForm
)

// LinearRegressionFitMethod configures the linear regression fitting algorithm.
type LinearRegressionFitMethod struct {
	iterations   uint32
	learningRate float64
	method       LinearRegressionType
}

// NewLinearRegressionFitMethod creates fitting options matching the Python binding.
func NewLinearRegressionFitMethod(iterations uint32, learningRate float64, method LinearRegressionType) (LinearRegressionFitMethod, error) {
	if method != GradientDescent && method != ClosedForm {
		return LinearRegressionFitMethod{}, fmt.Errorf("coolbox: unknown linear regression fit type %d", method)
	}
	if math.IsNaN(learningRate) || math.IsInf(learningRate, 0) {
		return LinearRegressionFitMethod{}, errors.New("coolbox: learning rate must be finite")
	}
	return LinearRegressionFitMethod{
		iterations:   iterations,
		learningRate: learningRate,
		method:       method,
	}, nil
}

// DefaultLinearRegressionFitMethod returns the Python binding's default options.
func DefaultLinearRegressionFitMethod() LinearRegressionFitMethod {
	return LinearRegressionFitMethod{
		iterations:   10000,
		learningRate: 0.01,
		method:       GradientDescent,
	}
}

func (method LinearRegressionFitMethod) NumIterations() uint32 {
	return method.iterations
}

func (method LinearRegressionFitMethod) LearningRate() float64 {
	return method.learningRate
}

func (method LinearRegressionFitMethod) Type() LinearRegressionType {
	return method.method
}

// LinearRegression is the native GLM linear regression implementation.
type LinearRegression struct {
	handle unsafe.Pointer
}

// NewLinearRegression constructs a model using method.
func NewLinearRegression(method LinearRegressionFitMethod) (*LinearRegression, error) {
	if method.method != GradientDescent && method.method != ClosedForm {
		return nil, fmt.Errorf("coolbox: unknown linear regression fit type %d", method.method)
	}
	handle := C.coolbox_glm_linear_regression_new(
		C.uint(method.iterations),
		C.double(method.learningRate),
		C.int(method.method),
	)
	if handle == nil {
		return nil, errors.New("coolbox: failed to create linear regression model")
	}
	return &LinearRegression{handle: handle}, nil
}

// Close releases the native model. It is safe to call Close more than once.
func (model *LinearRegression) Close() error {
	if model == nil || model.handle == nil {
		return nil
	}
	C.coolbox_glm_linear_regression_free(model.handle)
	model.handle = nil
	return nil
}

// Fit trains the model with a non-empty rectangular feature matrix and target vector.
func (model *LinearRegression) Fit(features [][]float64, targets []float64) error {
	handle, err := model.nativeHandle()
	if err != nil {
		return err
	}
	flat, rows, cols, err := flattenMatrix(features)
	if err != nil {
		return err
	}
	if len(targets) != rows {
		return errors.New("coolbox: target vector length must match the number of feature rows")
	}

	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	status := C.coolbox_glm_linear_regression_fit(
		handle,
		(*C.double)(unsafe.Pointer(&flat[0])),
		C.size_t(rows),
		C.size_t(cols),
		(*C.double)(unsafe.Pointer(&targets[0])),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if status == 0 {
		return nativeError(errorBuffer, "failed to fit linear regression model")
	}
	return nil
}

// Predict returns the model output for one feature vector.
func (model *LinearRegression) Predict(sample []float64) (float64, error) {
	handle, err := model.nativeHandle()
	if err != nil {
		return 0, err
	}
	if len(sample) == 0 {
		return 0, errors.New("coolbox: sample must be non-empty")
	}

	var prediction C.double
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	status := C.coolbox_glm_linear_regression_predict(
		handle,
		(*C.double)(unsafe.Pointer(&sample[0])),
		C.size_t(len(sample)),
		&prediction,
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if status == 0 {
		return 0, nativeError(errorBuffer, "failed to predict with linear regression model")
	}
	return float64(prediction), nil
}

// Coefficients returns the fitted weights and intercept.
func (model *LinearRegression) Coefficients() ([]float64, float64, error) {
	handle, err := model.nativeHandle()
	if err != nil {
		return nil, 0, err
	}
	count := int(C.coolbox_glm_linear_regression_feature_count(handle))
	if count == 0 {
		return nil, 0, errors.New("coolbox: linear regression model has not been fitted")
	}
	weights := make([]float64, count)
	var intercept C.double
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	status := C.coolbox_glm_linear_regression_coefficients(
		handle,
		(*C.double)(unsafe.Pointer(&weights[0])),
		C.size_t(len(weights)),
		&intercept,
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if status == 0 {
		return nil, 0, nativeError(errorBuffer, "failed to read linear regression coefficients")
	}
	return weights, float64(intercept), nil
}

func (model *LinearRegression) nativeHandle() (unsafe.Pointer, error) {
	if model == nil || model.handle == nil {
		return nil, errors.New("coolbox: linear regression model is closed")
	}
	return model.handle, nil
}

func flattenMatrix(matrix [][]float64) ([]float64, int, int, error) {
	if len(matrix) == 0 || len(matrix[0]) == 0 {
		return nil, 0, 0, errors.New("coolbox: feature matrix must be non-empty")
	}
	columns := len(matrix[0])
	flat := make([]float64, 0, len(matrix)*columns)
	for _, row := range matrix {
		if len(row) != columns {
			return nil, 0, 0, errors.New("coolbox: feature matrix rows must all have the same length")
		}
		flat = append(flat, row...)
	}
	return flat, len(matrix), columns, nil
}

func nativeErrorBuffer() *C.char {
	return (*C.char)(C.calloc(nativeErrorCapacity, 1))
}

func nativeError(buffer *C.char, fallback string) error {
	if buffer == nil {
		return errors.New(fallback)
	}
	message := C.GoString(buffer)
	if message == "" {
		message = fallback
	}
	return errors.New("coolbox: " + message)
}
