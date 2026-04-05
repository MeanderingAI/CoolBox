package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/common.h"
#include "abi/decision_tree.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

func NewDecisionTree(criterion string) (*DecisionTree, error) {
	if criterion == "" {
		criterion = SplitCriterionGini
	}
	cCriterion := C.CString(criterion)
	defer C.free(unsafe.Pointer(cCriterion))

	var cErr *C.char
	handle := C.coolbox_create_decision_tree(cCriterion, &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("decision tree handle is nil")
	}
	model := &DecisionTree{handle: handle}
	return withFinalizer(model, (*DecisionTree).finalize), nil
}

func (t *DecisionTree) Fit(x [][]int, y []int, maxDepth int) error {
	if t == nil || t.handle == nil {
		return fmt.Errorf("decision tree is nil")
	}
	xFlat, rows, cols, err := flattenIntMatrix(x)
	if err != nil {
		return err
	}
	if len(y) != rows {
		return fmt.Errorf("target vector length must match the number of feature rows")
	}
	xFlatC := toCIntSlice(xFlat)
	yC := toCIntSlice(y)

	var cErr *C.char
	ok := C.coolbox_decision_tree_fit(
		t.handle,
		cCIntPtr(xFlatC),
		C.size_t(rows),
		C.size_t(cols),
		cCIntPtr(yC),
		C.int(maxDepth),
		&cErr,
	)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("decision tree training failed")
	}
	return nil
}

func (t *DecisionTree) Predict(sample []int) (int, error) {
	if t == nil || t.handle == nil {
		return 0, fmt.Errorf("decision tree is nil")
	}
	sampleC := toCIntSlice(sample)
	var label C.int
	var cErr *C.char
	ok := C.coolbox_decision_tree_predict(t.handle, cCIntPtr(sampleC), C.size_t(len(sampleC)), &label, &cErr)
	if err := cError(cErr); err != nil {
		return 0, err
	}
	if ok == 0 {
		return 0, fmt.Errorf("decision tree prediction failed")
	}
	return int(label), nil
}

func (t *DecisionTree) finalize() { t.Close() }

func (t *DecisionTree) Close() {
	if t != nil && t.handle != nil {
		C.coolbox_free_decision_tree(t.handle)
		t.handle = nil
	}
}
