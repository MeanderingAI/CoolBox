package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/cbridge -I${SRCDIR}/../packages/ML/generalized_linear_model/headers -I${SRCDIR}/../packages/ML/decision_tree/headers -I${SRCDIR}/../packages/ML/bayesian_network_ai/headers -I${SRCDIR}/../packages/ML/hidden_markov_model/headers -I${SRCDIR}/../packages/ML/dimensionality_reduction/headers -I${SRCDIR}/../packages/ML/support_vector_machine/headers -I${SRCDIR}/../packages/ML/multi_arm_bandit/headers -I${SRCDIR}/../packages/MISC/metadata_management/headers -I${SRCDIR}/../../build/eigen-src -I${SRCDIR}/../packages/GRAPHICS/charts/headers
#cgo windows LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -lm -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
// Forward declarations for opaque FFI types.
#include "bridge_forward.h"
#include "bridge.h"
*/
import "C"

import (
	"fmt"
	"runtime"
)


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

const (
	FitMethodClosedForm     = "closed_form"
	FitMethodGradientDescent = "gradient_descent"

	SplitCriterionGini    = "gini"
	SplitCriterionEntropy = "entropy"

	KernelLinear     = "linear"
	KernelRBF        = "rbf"
	KernelPolynomial = "polynomial"
	KernelSigmoid    = "sigmoid"
)

type LinearRegression struct{ handle *C.CoolBoxLinearRegressionModel }
type DecisionTree struct{ handle *C.CoolBoxDecisionTreeModel }

type BayesianNetwork struct{ handle *C.CoolBoxBayesianNetworkModel }

type HMM struct {
	handle       *C.CoolBoxHMMModel
	states       int
	observations int
}

type PCA struct{ handle *C.CoolBoxPCAModel }

type SVM struct{ handle *C.CoolBoxSVMModel }

type BanditArm struct{ handle *C.CoolBoxBanditArmModel }

type BanditAgent struct{ handle *C.CoolBoxBanditAgentModel }

type BanditStats struct {
	TrueProbability      float64
	EstimatedProbability float64
	TimesPulled          int
}

type SimulationResult struct {
	Bandits []BanditStats
}

func boolToInt(value bool) int {
	if value {
		return 1
	}
	return 0
}
