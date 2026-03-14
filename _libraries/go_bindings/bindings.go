package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/../backages/ML/generalized_linear_model/headers -I${SRCDIR}/../backages/ML/decision_tree/headers -I${SRCDIR}/../backages/ML/bayesian_network_ai/headers -I${SRCDIR}/../backages/ML/hidden_markov_model/headers -I${SRCDIR}/../backages/ML/dimensionality_reduction/headers -I${SRCDIR}/../backages/ML/support_vector_machine/headers -I${SRCDIR}/../backages/ML/multi_arm_bandit/headers -I${SRCDIR}/../backages/MISC/metadata_management/headers -I${SRCDIR}/../../build/eigen-src
#cgo darwin LDFLAGS: -lc++
#cgo linux LDFLAGS: -lstdc++
#include <stdlib.h>
#include "bridge.h"
*/
import "C"

import (
	"fmt"
	"runtime"
	"unsafe"
)

const (
	FitMethodClosedForm      = "closed_form"
	FitMethodGradientDescent = "gradient_descent"

	SplitCriterionGini    = "gini"
	SplitCriterionEntropy = "entropy"

	KernelLinear     = "linear"
	KernelRBF        = "rbf"
	KernelPolynomial = "polynomial"
	KernelSigmoid    = "sigmoid"
)

type LinearRegression struct {
	handle *C.CoolBoxLinearRegressionModel
}

type DecisionTree struct {
	handle *C.CoolBoxDecisionTreeModel
}

type BayesianNetwork struct {
	handle *C.CoolBoxBayesianNetworkModel
}

type HMM struct {
	handle       *C.CoolBoxHMMModel
	states       int
	observations int
}

type PCA struct {
	handle *C.CoolBoxPCAModel
}

type SVM struct {
	handle *C.CoolBoxSVMModel
}

type BanditArm struct {
	handle *C.CoolBoxBanditArmModel
}

type BanditAgent struct {
	handle *C.CoolBoxBanditAgentModel
}

type BanditStats struct {
	TrueProbability      float64
	EstimatedProbability float64
	TimesPulled          int
}

type SimulationResult struct {
	Bandits []BanditStats
}

func cError(err *C.char) error {
	if err == nil {
		return nil
	}
	defer C.coolbox_free_string(err)
	return fmt.Errorf("%s", C.GoString(err))
}

func cDoublePtr(values []float64) *C.double {
	if len(values) == 0 {
		return nil
	}
	return (*C.double)(unsafe.Pointer(&values[0]))
}

func cCIntPtr(values []C.int) *C.int {
	if len(values) == 0 {
		return nil
	}
	return (*C.int)(unsafe.Pointer(&values[0]))
}

func toCIntSlice(values []int) []C.int {
	if len(values) == 0 {
		return nil
	}
	out := make([]C.int, len(values))
	for i, value := range values {
		out[i] = C.int(value)
	}
	return out
}

func fromCIntSlice(values []C.int) []int {
	if len(values) == 0 {
		return nil
	}
	out := make([]int, len(values))
	for i, value := range values {
		out[i] = int(value)
	}
	return out
}

func flattenFloatMatrix(x [][]float64) ([]float64, int, int, error) {
	if len(x) == 0 {
		return nil, 0, 0, fmt.Errorf("feature matrix must contain at least one row")
	}
	cols := len(x[0])
	if cols == 0 {
		return nil, 0, 0, fmt.Errorf("feature matrix must contain at least one column")
	}

	flat := make([]float64, 0, len(x)*cols)
	for _, row := range x {
		if len(row) != cols {
			return nil, 0, 0, fmt.Errorf("feature matrix rows must all have the same length")
		}
		flat = append(flat, row...)
	}

	return flat, len(x), cols, nil
}

func flattenIntMatrix(x [][]int) ([]int, int, int, error) {
	if len(x) == 0 {
		return nil, 0, 0, fmt.Errorf("feature matrix must contain at least one row")
	}
	cols := len(x[0])
	if cols == 0 {
		return nil, 0, 0, fmt.Errorf("feature matrix must contain at least one column")
	}

	flat := make([]int, 0, len(x)*cols)
	for _, row := range x {
		if len(row) != cols {
			return nil, 0, 0, fmt.Errorf("feature matrix rows must all have the same length")
		}
		flat = append(flat, row...)
	}

	return flat, len(x), cols, nil
}

func flattenIntSequences(sequences [][]int) ([]int, []C.size_t, error) {
	if len(sequences) == 0 {
		return nil, nil, fmt.Errorf("at least one observation sequence is required")
	}

	lengths := make([]C.size_t, len(sequences))
	flat := make([]int, 0)
	for i, seq := range sequences {
		if len(seq) == 0 {
			return nil, nil, fmt.Errorf("observation sequences must be non-empty")
		}
		lengths[i] = C.size_t(len(seq))
		flat = append(flat, seq...)
	}

	return flat, lengths, nil
}

func reshapeFloatMatrix(flat []float64, rows, cols int) [][]float64 {
	out := make([][]float64, rows)
	for i := 0; i < rows; i++ {
		start := i * cols
		row := make([]float64, cols)
		copy(row, flat[start:start+cols])
		out[i] = row
	}
	return out
}

func withFinalizer[T any](value *T, closeFn func(*T)) *T {
	runtime.SetFinalizer(value, closeFn)
	return value
}

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

func NewBanditArm(trueRewardProb float64) (*BanditArm, error) {
	var cErr *C.char
	handle := C.coolbox_create_bandit_arm(C.double(trueRewardProb), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit arm handle is nil")
	}
	model := &BanditArm{handle: handle}
	return withFinalizer(model, (*BanditArm).finalize), nil
}

func (b *BanditArm) Pull() (float64, error) {
	if b == nil || b.handle == nil {
		return 0, fmt.Errorf("bandit arm is nil")
	}
	var out C.double
	var cErr *C.char
	ok := C.coolbox_bandit_arm_pull(b.handle, &out, &cErr)
	if err := cError(cErr); err != nil {
		return 0, err
	}
	if ok == 0 {
		return 0, fmt.Errorf("bandit pull failed")
	}
	return float64(out), nil
}

func (b *BanditArm) Update(reward float64) error {
	if b == nil || b.handle == nil {
		return fmt.Errorf("bandit arm is nil")
	}
	var cErr *C.char
	ok := C.coolbox_bandit_arm_update(b.handle, C.double(reward), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("bandit update failed")
	}
	return nil
}

func (b *BanditArm) EstimatedProbability() float64 {
	if b == nil || b.handle == nil {
		return 0
	}
	return float64(C.coolbox_bandit_arm_estimated_prob(b.handle))
}

func (b *BanditArm) PullCount() int {
	if b == nil || b.handle == nil {
		return 0
	}
	return int(C.coolbox_bandit_arm_pull_count(b.handle))
}

func (b *BanditArm) TrueProbability() float64 {
	if b == nil || b.handle == nil {
		return 0
	}
	return float64(C.coolbox_bandit_arm_true_prob(b.handle))
}

func (b *BanditArm) finalize() { b.Close() }

func (b *BanditArm) Close() {
	if b != nil && b.handle != nil {
		C.coolbox_free_bandit_arm(b.handle)
		b.handle = nil
	}
}

func NewEpsilonGreedyAgent(trueProbs []float64, epsilon float64, seed int64) (*BanditAgent, error) {
	if len(trueProbs) == 0 {
		return nil, fmt.Errorf("at least one bandit arm is required")
	}
	var cErr *C.char
	handle := C.coolbox_create_epsilon_greedy_agent(cDoublePtr(trueProbs), C.size_t(len(trueProbs)), C.double(epsilon), C.double(0), C.longlong(seed), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit agent handle is nil")
	}
	model := &BanditAgent{handle: handle}
	return withFinalizer(model, (*BanditAgent).finalize), nil
}

func NewUCBAgent(trueProbs []float64, c float64) (*BanditAgent, error) {
	if len(trueProbs) == 0 {
		return nil, fmt.Errorf("at least one bandit arm is required")
	}
	var cErr *C.char
	handle := C.coolbox_create_ucb_agent(cDoublePtr(trueProbs), C.size_t(len(trueProbs)), C.double(c), C.double(0), C.longlong(0), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit agent handle is nil")
	}
	model := &BanditAgent{handle: handle}
	return withFinalizer(model, (*BanditAgent).finalize), nil
}

func NewThompsonSamplingAgent(trueProbs []float64, seed int64) (*BanditAgent, error) {
	if len(trueProbs) == 0 {
		return nil, fmt.Errorf("at least one bandit arm is required")
	}
	var cErr *C.char
	handle := C.coolbox_create_thompson_sampling_agent(cDoublePtr(trueProbs), C.size_t(len(trueProbs)), C.double(0), C.double(0), C.longlong(seed), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit agent handle is nil")
	}
	model := &BanditAgent{handle: handle}
	return withFinalizer(model, (*BanditAgent).finalize), nil
}

func NewDecayingEpsilonGreedyAgent(trueProbs []float64, initialEpsilon, decayRate float64, seed int64) (*BanditAgent, error) {
	if len(trueProbs) == 0 {
		return nil, fmt.Errorf("at least one bandit arm is required")
	}
	var cErr *C.char
	handle := C.coolbox_create_decaying_epsilon_greedy_agent(cDoublePtr(trueProbs), C.size_t(len(trueProbs)), C.double(initialEpsilon), C.double(decayRate), C.longlong(seed), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit agent handle is nil")
	}
	model := &BanditAgent{handle: handle}
	return withFinalizer(model, (*BanditAgent).finalize), nil
}

func (a *BanditAgent) RunSimulation(steps int) error {
	if a == nil || a.handle == nil {
		return fmt.Errorf("bandit agent is nil")
	}
	var cErr *C.char
	ok := C.coolbox_bandit_agent_run_simulation(a.handle, C.int(steps), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("bandit simulation failed")
	}
	return nil
}

func (a *BanditAgent) Results() (SimulationResult, error) {
	if a == nil || a.handle == nil {
		return SimulationResult{}, fmt.Errorf("bandit agent is nil")
	}
	count := int(C.coolbox_bandit_agent_arm_count(a.handle))
	trueProbs := make([]float64, count)
	estimated := make([]float64, count)
	pulls := make([]int, count)
	pullsC := make([]C.int, count)
	var cErr *C.char
	ok := C.coolbox_bandit_agent_get_results(
		a.handle,
		cDoublePtr(trueProbs),
		cDoublePtr(estimated),
		cCIntPtr(pullsC),
		C.size_t(count),
		&cErr,
	)
	if err := cError(cErr); err != nil {
		return SimulationResult{}, err
	}
	if ok == 0 {
		return SimulationResult{}, fmt.Errorf("collecting bandit results failed")
	}

	result := SimulationResult{Bandits: make([]BanditStats, count)}
	pulls = fromCIntSlice(pullsC)
	for i := range result.Bandits {
		result.Bandits[i] = BanditStats{
			TrueProbability:      trueProbs[i],
			EstimatedProbability: estimated[i],
			TimesPulled:          pulls[i],
		}
	}
	return result, nil
}

func (a *BanditAgent) finalize() { a.Close() }

func (a *BanditAgent) Close() {
	if a != nil && a.handle != nil {
		C.coolbox_free_bandit_agent(a.handle)
		a.handle = nil
	}
}

func boolToInt(value bool) int {
	if value {
		return 1
	}
	return 0
}
