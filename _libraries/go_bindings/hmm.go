package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/hmm.h"
*/
import "C"

import "unsafe"
import "fmt"

func NewHMM(states, observations int) (*HMM, error) {
	var cErr *C.char
	handle := C.coolbox_create_hmm(C.size_t(states), C.size_t(observations), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("hmm handle is nil")
	}
	model := &HMM{handle: handle, states: states, observations: observations}
	return withFinalizer(model, (*HMM).finalize), nil
}

func (h *HMM) SetInitialProbabilities(values []float64) error {
	if h == nil || h.handle == nil {
		return fmt.Errorf("hmm is nil")
	}
	if len(values) != h.states {
		return fmt.Errorf("initial probability vector length must match the number of states")
	}
	var cErr *C.char
	ok := C.coolbox_hmm_set_initial_probabilities(h.handle, cDoublePtr(values), C.size_t(len(values)), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("setting initial probabilities failed")
	}
	return nil
}

func (h *HMM) SetTransitionMatrix(values [][]float64) error {
	if h == nil || h.handle == nil {
		return fmt.Errorf("hmm is nil")
	}
	flat, rows, cols, err := flattenFloatMatrix(values)
	if err != nil {
		return err
	}
	var cErr *C.char
	ok := C.coolbox_hmm_set_transition_matrix(h.handle, cDoublePtr(flat), C.size_t(rows), C.size_t(cols), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("setting transition matrix failed")
	}
	return nil
}

func (h *HMM) SetEmissionMatrix(values [][]float64) error {
	if h == nil || h.handle == nil {
		return fmt.Errorf("hmm is nil")
	}
	flat, rows, cols, err := flattenFloatMatrix(values)
	if err != nil {
		return err
	}
	var cErr *C.char
	ok := C.coolbox_hmm_set_emission_matrix(h.handle, cDoublePtr(flat), C.size_t(rows), C.size_t(cols), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("setting emission matrix failed")
	}
	return nil
}

func (h *HMM) InitialProbabilities() ([]float64, error) {
	if h == nil || h.handle == nil {
		return nil, fmt.Errorf("hmm is nil")
	}
	values := make([]float64, h.states)
	var cErr *C.char
	ok := C.coolbox_hmm_get_initial_probabilities(h.handle, cDoublePtr(values), C.size_t(len(values)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting initial probabilities failed")
	}
	return values, nil
}

func (h *HMM) TransitionMatrix() ([][]float64, error) {
	if h == nil || h.handle == nil {
		return nil, fmt.Errorf("hmm is nil")
	}
	flat := make([]float64, h.states*h.states)
	var cErr *C.char
	ok := C.coolbox_hmm_get_transition_matrix(h.handle, cDoublePtr(flat), C.size_t(h.states), C.size_t(h.states), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting transition matrix failed")
	}
	return reshapeFloatMatrix(flat, h.states, h.states), nil
}

func (h *HMM) EmissionMatrix() ([][]float64, error) {
	if h == nil || h.handle == nil {
		return nil, fmt.Errorf("hmm is nil")
	}
	flat := make([]float64, h.states*h.observations)
	var cErr *C.char
	ok := C.coolbox_hmm_get_emission_matrix(h.handle, cDoublePtr(flat), C.size_t(h.states), C.size_t(h.observations), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("getting emission matrix failed")
	}
	return reshapeFloatMatrix(flat, h.states, h.observations), nil
}

func (h *HMM) LogLikelihood(observations []int) (float64, error) {
	if h == nil || h.handle == nil {
		return 0, fmt.Errorf("hmm is nil")
	}
	observationsC := toCIntSlice(observations)
	var out C.double
	var cErr *C.char
	ok := C.coolbox_hmm_log_likelihood(h.handle, cCIntPtr(observationsC), C.size_t(len(observationsC)), &out, &cErr)
	if err := cError(cErr); err != nil {
		return 0, err
	}
	if ok == 0 {
		return 0, fmt.Errorf("log likelihood failed")
	}
	return float64(out), nil
}

func (h *HMM) MostLikelyStates(observations []int) ([]int, error) {
	if h == nil || h.handle == nil {
		return nil, fmt.Errorf("hmm is nil")
	}
	observationsC := toCIntSlice(observations)
	outC := make([]C.int, len(observations))
	var cErr *C.char
	ok := C.coolbox_hmm_get_most_likely_states(h.handle, cCIntPtr(observationsC), C.size_t(len(observationsC)), cCIntPtr(outC), C.size_t(len(outC)), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("state decoding failed")
	}
	return fromCIntSlice(outC), nil
}

func (h *HMM) Train(sequences [][]int, maxIterations int, tolerance float64, smoothingFactor float64, seed uint32) error {
	if h == nil || h.handle == nil {
		return fmt.Errorf("hmm is nil")
	}
	flat, lengths, err := flattenIntSequences(sequences)
	if err != nil {
		return err
	}
	flatC := toCIntSlice(flat)
	var cErr *C.char
	ok := C.coolbox_hmm_train(
		h.handle,
		cCIntPtr(flatC),
		(*C.size_t)(unsafe.Pointer(&lengths[0])),
		C.size_t(len(lengths)),
		C.int(maxIterations),
		C.double(tolerance),
		C.double(smoothingFactor),
		C.uint32_t(seed),
		&cErr,
	)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("hmm training failed")
	}
	return nil
}

func (h *HMM) finalize() { h.Close() }

func (h *HMM) Close() {
	if h != nil && h.handle != nil {
		C.coolbox_free_hmm(h.handle)
		h.handle = nil
	}
}
