package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/bayesian_network.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

func NewBayesianNetwork() *BayesianNetwork {
	model := &BayesianNetwork{handle: C.coolbox_create_bayesian_network()}
	return withFinalizer(model, (*BayesianNetwork).finalize)
}

func (bn *BayesianNetwork) AddNode(name string, states []string) (int, error) {
	if bn == nil || bn.handle == nil {
		return 0, fmt.Errorf("bayesian network is nil")
	}
	if len(states) == 0 {
		return 0, fmt.Errorf("at least one state is required")
	}

	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	cStates := make([]*C.char, len(states))
	for i, state := range states {
		cStates[i] = C.CString(state)
		defer C.free(unsafe.Pointer(cStates[i]))
	}

	var outID C.int
	var cErr *C.char
	ok := C.coolbox_bayesian_network_add_node(
		bn.handle,
		cName,
		(**C.char)(unsafe.Pointer(&cStates[0])),
		C.size_t(len(cStates)),
		&outID,
		&cErr,
	)
	if err := cError(cErr); err != nil {
		return 0, err
	}
	if ok == 0 {
		return 0, fmt.Errorf("adding bayesian network node failed")
	}
	return int(outID), nil
}

func (bn *BayesianNetwork) AddEdge(parentID, childID int) error {
	if bn == nil || bn.handle == nil {
		return fmt.Errorf("bayesian network is nil")
	}
	var cErr *C.char
	ok := C.coolbox_bayesian_network_add_edge(bn.handle, C.int(parentID), C.int(childID), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("adding bayesian network edge failed")
	}
	return nil
}

func (bn *BayesianNetwork) SetCPT(nodeID int, values []float64) error {
	if bn == nil || bn.handle == nil {
		return fmt.Errorf("bayesian network is nil")
	}
	var cErr *C.char
	ok := C.coolbox_bayesian_network_set_cpt(bn.handle, C.int(nodeID), cDoublePtr(values), C.size_t(len(values)), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("setting CPT failed")
	}
	return nil
}

func (bn *BayesianNetwork) Query(queryNode int, evidence map[int]int) ([]float64, error) {
	if bn == nil || bn.handle == nil {
		return nil, fmt.Errorf("bayesian network is nil")
	}
	stateCount, err := bn.NodeStateCount(queryNode)
	if err != nil {
		return nil, err
	}

	nodeIDs := make([]int, 0, len(evidence))
	stateIDs := make([]int, 0, len(evidence))
	for nodeID, stateID := range evidence {
		nodeIDs = append(nodeIDs, nodeID)
		stateIDs = append(stateIDs, stateID)
	}
	nodeIDsC := toCIntSlice(nodeIDs)
	stateIDsC := toCIntSlice(stateIDs)

	out := make([]float64, stateCount)
	var cErr *C.char
	ok := C.coolbox_bayesian_network_query(
		bn.handle,
		C.int(queryNode),
		cCIntPtr(nodeIDsC),
		cCIntPtr(stateIDsC),
		C.size_t(len(nodeIDs)),
		cDoublePtr(out),
		C.size_t(len(out)),
		&cErr,
	)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if ok == 0 {
		return nil, fmt.Errorf("bayesian network query failed")
	}
	return out, nil
}

func (bn *BayesianNetwork) NumNodes() int {
	if bn == nil || bn.handle == nil {
		return 0
	}
	return int(C.coolbox_bayesian_network_num_nodes(bn.handle))
}

func (bn *BayesianNetwork) GetNodeID(name string) (int, error) {
	if bn == nil || bn.handle == nil {
		return 0, fmt.Errorf("bayesian network is nil")
	}
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	var outID C.int
	var cErr *C.char
	ok := C.coolbox_bayesian_network_get_node_id(bn.handle, cName, &outID, &cErr)
	if err := cError(cErr); err != nil {
		return 0, err
	}
	if ok == 0 {
		return 0, fmt.Errorf("lookup failed")
	}
	return int(outID), nil
}

func (bn *BayesianNetwork) NodeStateCount(nodeID int) (int, error) {
	if bn == nil || bn.handle == nil {
		return 0, fmt.Errorf("bayesian network is nil")
	}
	var count C.size_t
	var cErr *C.char
	ok := C.coolbox_bayesian_network_get_node_state_count(bn.handle, C.int(nodeID), &count, &cErr)
	if err := cError(cErr); err != nil {
		return 0, err
	}
	if ok == 0 {
		return 0, fmt.Errorf("state count lookup failed")
	}
	return int(count), nil
}

func (bn *BayesianNetwork) finalize() { bn.Close() }

func (bn *BayesianNetwork) Close() {
	if bn != nil && bn.handle != nil {
		C.coolbox_free_bayesian_network(bn.handle)
		bn.handle = nil
	}
}
