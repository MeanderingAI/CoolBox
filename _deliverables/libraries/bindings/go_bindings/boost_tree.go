package coolbox

/*
#include <stdlib.h>
#include <stdint.h>
void* coolbox_boost_tree_regressor_new(unsigned int num_estimators, double learning_rate, unsigned int max_depth, char* error, size_t error_capacity);
void coolbox_boost_tree_regressor_free(void* handle);
int coolbox_boost_tree_regressor_fit(void* handle, const double* features, size_t rows, size_t columns, const double* targets, char* error, size_t error_capacity);
int coolbox_boost_tree_regressor_predict(void* handle, const double* sample, size_t columns, double* prediction, char* error, size_t error_capacity);
int coolbox_boost_tree_regressor_predict_batch(void* handle, const double* features, size_t rows, size_t columns, double* predictions, char* error, size_t error_capacity);
void* coolbox_boost_tree_classifier_new(unsigned int num_estimators, double learning_rate, unsigned int max_depth, char* error, size_t error_capacity);
void coolbox_boost_tree_classifier_free(void* handle);
int coolbox_boost_tree_classifier_fit(void* handle, const double* features, size_t rows, size_t columns, const int* labels, char* error, size_t error_capacity);
int coolbox_boost_tree_classifier_predict(void* handle, const double* sample, size_t columns, int* prediction, char* error, size_t error_capacity);
int coolbox_boost_tree_classifier_predict_proba(void* handle, const double* sample, size_t columns, double* probabilities, size_t capacity, char* error, size_t error_capacity);
size_t coolbox_boost_tree_classifier_class_count(void* handle, char* error, size_t error_capacity);
int coolbox_boost_tree_classifier_copy_classes(void* handle, int* classes, size_t capacity, char* error, size_t error_capacity);
*/
import "C"

import (
	"errors"
	"fmt"
	"math"
	"unsafe"
)

// BoostTreeParameters configures regression and classification gradient boosting.
type BoostTreeParameters struct {
	NumEstimators uint32
	LearningRate  float64
	MaxDepth      uint32
}

// DefaultBoostTreeParameters returns the native boosted-tree defaults.
func DefaultBoostTreeParameters() BoostTreeParameters {
	return BoostTreeParameters{NumEstimators: 100, LearningRate: 0.1, MaxDepth: 3}
}

// BoostTree is a native gradient-boosted decision-tree regressor.
type BoostTree struct {
	handle unsafe.Pointer
	params BoostTreeParameters
}

// NewBoostTree creates a regressor using parameters. A zero-value parameter
// struct is not implicitly replaced with defaults; use DefaultBoostTreeParameters.
func NewBoostTree(parameters BoostTreeParameters) (*BoostTree, error) {
	if err := validateBoostTreeParameters(parameters); err != nil {
		return nil, err
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	handle := C.coolbox_boost_tree_regressor_new(
		C.uint(parameters.NumEstimators),
		C.double(parameters.LearningRate),
		C.uint(parameters.MaxDepth),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if handle == nil {
		return nil, nativeError(errorBuffer, "failed to create boosted-tree regressor")
	}
	return &BoostTree{handle: handle, params: parameters}, nil
}

// Parameters returns the immutable configuration used to construct this model.
func (model *BoostTree) Parameters() BoostTreeParameters {
	if model == nil {
		return BoostTreeParameters{}
	}
	return model.params
}

// Close releases the native boosted-tree regressor.
func (model *BoostTree) Close() error {
	if model != nil && model.handle != nil {
		C.coolbox_boost_tree_regressor_free(model.handle)
		model.handle = nil
	}
	return nil
}

// Fit trains the regressor on a rectangular numeric feature matrix.
func (model *BoostTree) Fit(features [][]float64, targets []float64) error {
	if model == nil || model.handle == nil {
		return errors.New("coolbox: boosted-tree regressor is closed")
	}
	flat, rows, columns, err := flattenMatrix(features)
	if err != nil {
		return err
	}
	if len(targets) != rows {
		return errors.New("coolbox: target count must match the number of feature rows")
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_boost_tree_regressor_fit(
		model.handle,
		(*C.double)(unsafe.Pointer(&flat[0])),
		C.size_t(rows),
		C.size_t(columns),
		(*C.double)(unsafe.Pointer(&targets[0])),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return nativeError(errorBuffer, "failed to fit boosted-tree regressor")
	}
	return nil
}

// Predict estimates the target for one numeric feature vector.
func (model *BoostTree) Predict(sample []float64) (float64, error) {
	if model == nil || model.handle == nil {
		return 0, errors.New("coolbox: boosted-tree regressor is closed")
	}
	if len(sample) == 0 {
		return 0, errors.New("coolbox: prediction sample must not be empty")
	}
	var prediction C.double
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_boost_tree_regressor_predict(
		model.handle,
		(*C.double)(unsafe.Pointer(&sample[0])),
		C.size_t(len(sample)),
		&prediction,
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return 0, nativeError(errorBuffer, "failed to predict with boosted-tree regressor")
	}
	return float64(prediction), nil
}

// PredictBatch estimates targets for a rectangular feature matrix.
func (model *BoostTree) PredictBatch(features [][]float64) ([]float64, error) {
	if model == nil || model.handle == nil {
		return nil, errors.New("coolbox: boosted-tree regressor is closed")
	}
	flat, rows, columns, err := flattenMatrix(features)
	if err != nil {
		return nil, err
	}
	values := make([]C.double, rows)
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_boost_tree_regressor_predict_batch(
		model.handle,
		(*C.double)(unsafe.Pointer(&flat[0])),
		C.size_t(rows),
		C.size_t(columns),
		&values[0],
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return nil, nativeError(errorBuffer, "failed to predict with boosted-tree regressor")
	}
	result := make([]float64, len(values))
	for index, value := range values {
		result[index] = float64(value)
	}
	return result, nil
}

// BoostTreeClassifier is a native gradient-boosted decision-tree classifier.
type BoostTreeClassifier struct {
	handle unsafe.Pointer
	params BoostTreeParameters
}

// NewBoostTreeClassifier creates a classifier using parameters.
func NewBoostTreeClassifier(parameters BoostTreeParameters) (*BoostTreeClassifier, error) {
	if err := validateBoostTreeParameters(parameters); err != nil {
		return nil, err
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	handle := C.coolbox_boost_tree_classifier_new(
		C.uint(parameters.NumEstimators),
		C.double(parameters.LearningRate),
		C.uint(parameters.MaxDepth),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if handle == nil {
		return nil, nativeError(errorBuffer, "failed to create boosted-tree classifier")
	}
	return &BoostTreeClassifier{handle: handle, params: parameters}, nil
}

// Parameters returns the immutable configuration used to construct this model.
func (model *BoostTreeClassifier) Parameters() BoostTreeParameters {
	if model == nil {
		return BoostTreeParameters{}
	}
	return model.params
}

// Close releases the native boosted-tree classifier.
func (model *BoostTreeClassifier) Close() error {
	if model != nil && model.handle != nil {
		C.coolbox_boost_tree_classifier_free(model.handle)
		model.handle = nil
	}
	return nil
}

// Fit trains the classifier with arbitrary int32 class labels.
func (model *BoostTreeClassifier) Fit(features [][]float64, labels []int) error {
	if model == nil || model.handle == nil {
		return errors.New("coolbox: boosted-tree classifier is closed")
	}
	flat, rows, columns, err := flattenMatrix(features)
	if err != nil {
		return err
	}
	if len(labels) != rows {
		return errors.New("coolbox: label count must match the number of feature rows")
	}
	cLabels := make([]C.int, len(labels))
	for index, label := range labels {
		if int(int32(label)) != label {
			return fmt.Errorf("coolbox: class label at index %d does not fit in a 32-bit integer", index)
		}
		cLabels[index] = C.int(label)
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_boost_tree_classifier_fit(
		model.handle,
		(*C.double)(unsafe.Pointer(&flat[0])),
		C.size_t(rows),
		C.size_t(columns),
		&cLabels[0],
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return nativeError(errorBuffer, "failed to fit boosted-tree classifier")
	}
	return nil
}

// Predict returns the most likely class for one sample.
func (model *BoostTreeClassifier) Predict(sample []float64) (int, error) {
	if model == nil || model.handle == nil {
		return 0, errors.New("coolbox: boosted-tree classifier is closed")
	}
	if len(sample) == 0 {
		return 0, errors.New("coolbox: prediction sample must not be empty")
	}
	var prediction C.int
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	if C.coolbox_boost_tree_classifier_predict(
		model.handle,
		(*C.double)(unsafe.Pointer(&sample[0])),
		C.size_t(len(sample)),
		&prediction,
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return 0, nativeError(errorBuffer, "failed to predict with boosted-tree classifier")
	}
	return int(prediction), nil
}

// PredictProba returns class probabilities ordered as in Classes().
func (model *BoostTreeClassifier) PredictProba(sample []float64) ([]float64, error) {
	if model == nil || model.handle == nil {
		return nil, errors.New("coolbox: boosted-tree classifier is closed")
	}
	if len(sample) == 0 {
		return nil, errors.New("coolbox: prediction sample must not be empty")
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	count := C.coolbox_boost_tree_classifier_class_count(
		model.handle, errorBuffer, C.size_t(nativeErrorCapacity))
	if count == 0 {
		return nil, nativeError(errorBuffer, "failed to get boosted-tree classes")
	}
	probabilities := make([]C.double, int(count))
	if C.coolbox_boost_tree_classifier_predict_proba(
		model.handle,
		(*C.double)(unsafe.Pointer(&sample[0])),
		C.size_t(len(sample)),
		&probabilities[0],
		count,
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	) == 0 {
		return nil, nativeError(errorBuffer, "failed to predict probabilities")
	}
	result := make([]float64, len(probabilities))
	for index, probability := range probabilities {
		result[index] = float64(probability)
	}
	return result, nil
}

// Classes returns fitted class labels in the same order as PredictProba.
func (model *BoostTreeClassifier) Classes() ([]int, error) {
	if model == nil || model.handle == nil {
		return nil, errors.New("coolbox: boosted-tree classifier is closed")
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	count := C.coolbox_boost_tree_classifier_class_count(
		model.handle, errorBuffer, C.size_t(nativeErrorCapacity))
	if count == 0 {
		return nil, nativeError(errorBuffer, "failed to get boosted-tree classes")
	}
	values := make([]C.int, int(count))
	if C.coolbox_boost_tree_classifier_copy_classes(
		model.handle, &values[0], count, errorBuffer, C.size_t(nativeErrorCapacity)) == 0 {
		return nil, nativeError(errorBuffer, "failed to copy boosted-tree classes")
	}
	classes := make([]int, len(values))
	for index, value := range values {
		classes[index] = int(value)
	}
	return classes, nil
}

func validateBoostTreeParameters(parameters BoostTreeParameters) error {
	if parameters.NumEstimators == 0 || parameters.LearningRate <= 0 ||
		math.IsNaN(parameters.LearningRate) || math.IsInf(parameters.LearningRate, 0) ||
		parameters.MaxDepth > math.MaxInt32 {
		return errors.New("coolbox: boosted-tree parameters require positive estimators and learning rate and a valid depth")
	}
	return nil
}
