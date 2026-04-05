package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/common.h"
#include "abi/linear_regression.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

func FitLinearRegression(x [][]float64, y []float64, method string, iterations uint32, learningRate float64) (*LinearRegression, error) {
	xFlat, rows, cols, err := flattenFloatMatrix(x)
	if err != nil {
		return nil, err
	}
	if len(y) != rows {
		return nil, fmt.Errorf("target vector length must match the number of feature rows")
	}
	if method == "" {
		method = FitMethodClosedForm
	}

	cMethod := C.CString(method)
	defer C.free(unsafe.Pointer(cMethod))

	var cErr *C.char
	handle := C.coolbox_fit_linear_regression(
		cDoublePtr(xFlat),
		C.size_t(rows),
		C.size_t(cols),
		cDoublePtr(y),
		cMethod,
		C.uint32_t(iterations),
		C.double(learningRate),
		&cErr,
	)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("fit returned a nil model handle")
	}

	model := &LinearRegression{handle: handle}
	return withFinalizer(model, (*LinearRegression).finalize), nil
}

func (m *LinearRegression) Predict(x [][]float64) ([]float64, error) {
	if m == nil || m.handle == nil {
		return nil, fmt.Errorf("model is nil")
	}
	xFlat, rows, cols, err := flattenFloatMatrix(x)
	if err != nil {
		return nil, err
	}
	predictions := make([]float64, rows)

	var cErr *C.char
	ok := C.coolbox_predict_linear_regression(
		m.handle,
		cDoublePtr(xFlat),
		C.size_t(rows),
		C.size_t(cols),
		cDoublePtr(predictions),
		&cErr,
	)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("prediction failed")
	}
	return predictions, nil
}

func (m *LinearRegression) Weights() []float64 {
	if m == nil || m.handle == nil {
		return nil
	}
	count := int(C.coolbox_linear_regression_feature_count(m.handle))
	weights := make([]float64, count)
	for i := 0; i < count; i++ {
		weights[i] = float64(C.coolbox_linear_regression_weight_at(m.handle, C.size_t(i)))
	}
	return weights
}

func (m *LinearRegression) Intercept() float64 {
	if m == nil || m.handle == nil {
		return 0
	}
	return float64(C.coolbox_linear_regression_intercept(m.handle))
}

func (m *LinearRegression) FeatureCount() int {
	if m == nil || m.handle == nil {
		return 0
	}
	return int(C.coolbox_linear_regression_feature_count(m.handle))
}

func (m *LinearRegression) MethodName() string {
	if m == nil || m.handle == nil {
		return ""
	}
	return C.GoString(C.coolbox_linear_regression_method_name(m.handle))
}

func (m *LinearRegression) finalize() { m.Close() }

func (m *LinearRegression) Close() {
	if m != nil && m.handle != nil {
		C.coolbox_free_linear_regression(m.handle)
		m.handle = nil
	}
}
