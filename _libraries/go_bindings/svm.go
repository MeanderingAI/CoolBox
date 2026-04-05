package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/svm.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

func NewSVM(kernelType string, param1, param2 float64, param3 int) (*SVM, error) {
	if kernelType == "" {
		kernelType = KernelLinear
	}
	cKernel := C.CString(kernelType)
	defer C.free(unsafe.Pointer(cKernel))

	var cErr *C.char
	handle := C.coolbox_create_svm(cKernel, C.double(param1), C.double(param2), C.int(param3), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("svm handle is nil")
	}
	model := &SVM{handle: handle}
	return withFinalizer(model, (*SVM).finalize), nil
}

func (s *SVM) Fit(x [][]float64, y []float64) error {
	if s == nil || s.handle == nil {
		return fmt.Errorf("svm is nil")
	}
	xFlat, rows, cols, err := flattenFloatMatrix(x)
	if err != nil {
		return err
	}
	if len(y) != rows {
		return fmt.Errorf("target vector length must match the number of feature rows")
	}
	var cErr *C.char
	ok := C.coolbox_svm_fit(s.handle, cDoublePtr(xFlat), C.size_t(rows), C.size_t(cols), cDoublePtr(y), C.size_t(len(y)), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("svm fit failed")
	}
	return nil
}

func (s *SVM) Predict(sample []float64) (float64, error) {
	if s == nil || s.handle == nil {
		return 0, fmt.Errorf("svm is nil")
	}
	var out C.double
	var cErr *C.char
	ok := C.coolbox_svm_predict(s.handle, cDoublePtr(sample), C.size_t(len(sample)), &out, &cErr)
	if err := cError(cErr); err != nil {
		return 0, err
	}
	if ok == 0 {
		return 0, fmt.Errorf("svm prediction failed")
	}
	return float64(out), nil
}

func (s *SVM) finalize() { s.Close() }

func (s *SVM) Close() {
	if s != nil && s.handle != nil {
		C.coolbox_free_svm(s.handle)
		s.handle = nil
	}
}
