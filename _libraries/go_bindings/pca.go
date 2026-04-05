package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/pca.h"
*/
import "C"

import "fmt"

func NewPCA(nComponents int, center bool, scale bool) (*PCA, error) {
	var cErr *C.char
	handle := C.coolbox_create_pca(C.int(nComponents), C.int(boolToInt(center)), C.int(boolToInt(scale)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("pca handle is nil")
	}
	model := &PCA{handle: handle}
	return withFinalizer(model, (*PCA).finalize), nil
}

func (p *PCA) Fit(x [][]float64) error {
	if p == nil || p.handle == nil {
		return fmt.Errorf("pca is nil")
	}
	xFlat, rows, cols, err := flattenFloatMatrix(x)
	if err != nil {
		return err
	}
	var cErr *C.char
	ok := C.coolbox_pca_fit(p.handle, cDoublePtr(xFlat), C.size_t(rows), C.size_t(cols), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("pca fit failed")
	}
	return nil
}

func (p *PCA) Transform(x [][]float64) ([][]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	xFlat, rows, cols, err := flattenFloatMatrix(x)
	if err != nil {
		return nil, err
	}
	components := p.ComponentCount()
	out := make([]float64, rows*components)
	var cErr *C.char
	ok := C.coolbox_pca_transform(p.handle, cDoublePtr(xFlat), C.size_t(rows), C.size_t(cols), cDoublePtr(out), C.size_t(len(out)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("pca transform failed")
	}
	return reshapeFloatMatrix(out, rows, components), nil
}

func (p *PCA) FitTransform(x [][]float64) ([][]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	xFlat, rows, cols, err := flattenFloatMatrix(x)
	if err != nil {
		return nil, err
	}
	out := make([]float64, rows*cols)
	var outCols C.size_t
	var cErr *C.char
	ok := C.coolbox_pca_fit_transform(p.handle, cDoublePtr(xFlat), C.size_t(rows), C.size_t(cols), cDoublePtr(out), C.size_t(len(out)), &outCols, &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("pca fit_transform failed")
	}
	components := int(outCols)
	return reshapeFloatMatrix(out[:rows*components], rows, components), nil
}

func (p *PCA) InverseTransform(x [][]float64) ([][]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	xFlat, rows, cols, err := flattenFloatMatrix(x)
	if err != nil {
		return nil, err
	}
	features := p.FeatureCount()
	out := make([]float64, rows*features)
	var cErr *C.char
	ok := C.coolbox_pca_inverse_transform(p.handle, cDoublePtr(xFlat), C.size_t(rows), C.size_t(cols), cDoublePtr(out), C.size_t(len(out)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("pca inverse_transform failed")
	}
	return reshapeFloatMatrix(out, rows, features), nil
}

func (p *PCA) Components() ([][]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	rows := p.FeatureCount()
	cols := p.ComponentCount()
	out := make([]float64, rows*cols)
	var cErr *C.char
	ok := C.coolbox_pca_get_components(p.handle, cDoublePtr(out), C.size_t(len(out)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting pca components failed")
	}
	return reshapeFloatMatrix(out, rows, cols), nil
}

func (p *PCA) ExplainedVariance() ([]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	count := p.ComponentCount()
	out := make([]float64, count)
	var cErr *C.char
	ok := C.coolbox_pca_get_explained_variance(p.handle, cDoublePtr(out), C.size_t(len(out)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting explained variance failed")
	}
	return out, nil
}

func (p *PCA) ExplainedVarianceRatio() ([]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	count := p.ComponentCount()
	out := make([]float64, count)
	var cErr *C.char
	ok := C.coolbox_pca_get_explained_variance_ratio(p.handle, cDoublePtr(out), C.size_t(len(out)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting explained variance ratio failed")
	}
	return out, nil
}

func (p *PCA) SingularValues() ([]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	count := p.ComponentCount()
	out := make([]float64, count)
	var cErr *C.char
	ok := C.coolbox_pca_get_singular_values(p.handle, cDoublePtr(out), C.size_t(len(out)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting singular values failed")
	}
	return out, nil
}

func (p *PCA) Mean() ([]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	count := p.FeatureCount()
	out := make([]float64, count)
	var cErr *C.char
	ok := C.coolbox_pca_get_mean(p.handle, cDoublePtr(out), C.size_t(len(out)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting pca mean failed")
	}
	return out, nil
}

func (p *PCA) Scale() ([]float64, error) {
	if p == nil || p.handle == nil {
		return nil, fmt.Errorf("pca is nil")
	}
	count := p.FeatureCount()
	out := make([]float64, count)
	var cErr *C.char
	ok := C.coolbox_pca_get_scale(p.handle, cDoublePtr(out), C.size_t(len(out)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting pca scale failed")
	}
	return out, nil
}

func (p *PCA) ComponentCount() int {
	if p == nil || p.handle == nil {
		return 0
	}
	return int(C.coolbox_pca_component_count(p.handle))
}

func (p *PCA) FeatureCount() int {
	if p == nil || p.handle == nil {
		return 0
	}
	return int(C.coolbox_pca_feature_count(p.handle))
}

func (p *PCA) IsFitted() bool {
	return p != nil && p.handle != nil && C.coolbox_pca_is_fitted(p.handle) != 0
}

func (p *PCA) finalize() { p.Close() }

func (p *PCA) Close() {
	if p != nil && p.handle != nil {
		C.coolbox_free_pca(p.handle)
		p.handle = nil
	}
}
