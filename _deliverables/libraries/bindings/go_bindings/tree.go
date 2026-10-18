package coolbox

/*
#include <stdlib.h>
void* coolbox_tree_classifier_new(int criterion);
void coolbox_tree_classifier_free(void* handle);
int coolbox_tree_classifier_fit(void* handle, const int* features, size_t rows, size_t columns, const int* labels, int max_depth, char* error, size_t error_capacity);
int coolbox_tree_classifier_predict(void* handle, const int* sample, size_t columns, int* prediction, char* error, size_t error_capacity);
void* coolbox_tree_regressor_new(void);
void coolbox_tree_regressor_free(void* handle);
int coolbox_tree_regressor_fit(void* handle, const double* features, size_t rows, size_t columns, const double* targets, int max_depth, char* error, size_t error_capacity);
int coolbox_tree_regressor_predict(void* handle, const double* sample, size_t columns, double* prediction, char* error, size_t error_capacity);
*/
import "C"

import (
	"errors"
	"fmt"
	"unsafe"
)

// SplitCriterion controls the impurity calculation used by a decision tree.
type SplitCriterion int

const (
	Gini SplitCriterion = iota
	Entropy
)

// DecisionTree is a native categorical decision-tree classifier.
type DecisionTree struct {
	handle       unsafe.Pointer
	featureCount int
}

// NewDecisionTree creates a classifier using criterion.
func NewDecisionTree(criterion SplitCriterion) (*DecisionTree, error) {
	if criterion != Gini && criterion != Entropy {
		return nil, fmt.Errorf("coolbox: invalid decision-tree split criterion %d", criterion)
	}
	handle := C.coolbox_tree_classifier_new(C.int(criterion))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create decision tree")
	}
	return &DecisionTree{handle: handle}, nil
}

// Close releases the native classifier.
func (tree *DecisionTree) Close() error {
	if tree == nil || tree.handle == nil {
		return nil
	}
	C.coolbox_tree_classifier_free(tree.handle)
	tree.handle = nil
	return nil
}

// Fit trains the classifier on integer-valued categorical features.
func (tree *DecisionTree) Fit(features [][]int, labels []int, maxDepth int) error {
	if tree == nil || tree.handle == nil {
		return errors.New("coolbox: decision tree is closed")
	}
	flat, rows, columns, err := flattenIntMatrix(features)
	if err != nil {
		return err
	}
	if len(labels) != rows {
		return errors.New("coolbox: label count must match the number of feature rows")
	}
	if maxDepth < 0 {
		return errors.New("coolbox: max depth cannot be negative")
	}
	cFeatures := make([]C.int, len(flat))
	for index, value := range flat {
		if int(int32(value)) != value {
			return errors.New("coolbox: categorical feature values must fit in a 32-bit integer")
		}
		cFeatures[index] = C.int(value)
	}
	cLabels := make([]C.int, len(labels))
	for index, value := range labels {
		if int(int32(value)) != value {
			return errors.New("coolbox: class labels must fit in a 32-bit integer")
		}
		cLabels[index] = C.int(value)
	}
	if int(int32(maxDepth)) != maxDepth {
		return errors.New("coolbox: max depth must fit in a 32-bit integer")
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	status := C.coolbox_tree_classifier_fit(
		tree.handle,
		&cFeatures[0],
		C.size_t(rows),
		C.size_t(columns),
		&cLabels[0],
		C.int(maxDepth),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if status == 0 {
		return nativeError(errorBuffer, "failed to fit decision tree")
	}
	tree.featureCount = columns
	return nil
}

// Predict classifies one integer-valued feature vector.
func (tree *DecisionTree) Predict(sample []int) (int, error) {
	if tree == nil || tree.handle == nil {
		return 0, errors.New("coolbox: decision tree is closed")
	}
	if len(sample) == 0 || len(sample) != tree.featureCount {
		return 0, errors.New("coolbox: sample feature count must match the fitted model")
	}
	cSample := make([]C.int, len(sample))
	for index, value := range sample {
		cSample[index] = C.int(value)
	}
	var prediction C.int
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	status := C.coolbox_tree_classifier_predict(
		tree.handle,
		&cSample[0],
		C.size_t(len(cSample)),
		&prediction,
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if status == 0 {
		return 0, nativeError(errorBuffer, "failed to predict with decision tree")
	}
	return int(prediction), nil
}

// DecisionTreeRegressor is a native regression tree for numeric features.
type DecisionTreeRegressor struct {
	handle       unsafe.Pointer
	featureCount int
}

// NewDecisionTreeRegressor creates a regression tree.
func NewDecisionTreeRegressor() (*DecisionTreeRegressor, error) {
	handle := C.coolbox_tree_regressor_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create decision tree regressor")
	}
	return &DecisionTreeRegressor{handle: handle}, nil
}

// Close releases the native regression tree.
func (tree *DecisionTreeRegressor) Close() error {
	if tree == nil || tree.handle == nil {
		return nil
	}
	C.coolbox_tree_regressor_free(tree.handle)
	tree.handle = nil
	return nil
}

// Fit trains the regression tree on a rectangular feature matrix and targets.
func (tree *DecisionTreeRegressor) Fit(features [][]float64, targets []float64, maxDepth int) error {
	if tree == nil || tree.handle == nil {
		return errors.New("coolbox: decision tree regressor is closed")
	}
	flat, rows, columns, err := flattenMatrix(features)
	if err != nil {
		return err
	}
	if len(targets) != rows {
		return errors.New("coolbox: target count must match the number of feature rows")
	}
	if maxDepth < 0 {
		return errors.New("coolbox: max depth cannot be negative")
	}
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	status := C.coolbox_tree_regressor_fit(
		tree.handle,
		(*C.double)(unsafe.Pointer(&flat[0])),
		C.size_t(rows),
		C.size_t(columns),
		(*C.double)(unsafe.Pointer(&targets[0])),
		C.int(maxDepth),
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if status == 0 {
		return nativeError(errorBuffer, "failed to fit decision tree regressor")
	}
	tree.featureCount = columns
	return nil
}

// Predict returns the estimated target for one feature vector.
func (tree *DecisionTreeRegressor) Predict(sample []float64) (float64, error) {
	if tree == nil || tree.handle == nil {
		return 0, errors.New("coolbox: decision tree regressor is closed")
	}
	if len(sample) == 0 || len(sample) != tree.featureCount {
		return 0, errors.New("coolbox: sample feature count must match the fitted model")
	}
	var prediction C.double
	errorBuffer := nativeErrorBuffer()
	defer C.free(unsafe.Pointer(errorBuffer))
	status := C.coolbox_tree_regressor_predict(
		tree.handle,
		(*C.double)(unsafe.Pointer(&sample[0])),
		C.size_t(len(sample)),
		&prediction,
		errorBuffer,
		C.size_t(nativeErrorCapacity),
	)
	if status == 0 {
		return 0, nativeError(errorBuffer, "failed to predict with decision tree regressor")
	}
	return float64(prediction), nil
}

func flattenIntMatrix(matrix [][]int) ([]int, int, int, error) {
	if len(matrix) == 0 || len(matrix[0]) == 0 {
		return nil, 0, 0, errors.New("coolbox: feature matrix must be non-empty")
	}
	columns := len(matrix[0])
	flat := make([]int, 0, len(matrix)*columns)
	for _, row := range matrix {
		if len(row) != columns {
			return nil, 0, 0, errors.New("coolbox: feature matrix rows must all have the same length")
		}
		flat = append(flat, row...)
	}
	return flat, len(matrix), columns, nil
}
