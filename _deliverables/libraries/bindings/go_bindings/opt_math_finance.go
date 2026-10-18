package coolbox

/*
#include "opt_math_finance_bridge.h"
#include <stdlib.h>
*/
import "C"

import (
	"errors"
	"fmt"
	"math"
	"runtime/cgo"
	"unsafe"
)

const optimizationErrorCapacity = 1024

func optimizationNativeError(buffer *[optimizationErrorCapacity]C.char) error {
	message := C.GoString(&buffer[0])
	if message == "" {
		message = "native optimization/math/finance operation failed"
	}
	return errors.New(message)
}

func callNative(ok C.int, buffer *[optimizationErrorCapacity]C.char) error {
	if ok == 0 {
		return optimizationNativeError(buffer)
	}
	return nil
}

type SearchOptimizerType int

const (
	GeneticSearchOptimizer SearchOptimizerType = iota
	SimulatedAnnealingOptimizer
	HillClimbingOptimizer
	QuantumControlBayesianOptimizer
)

type GeneticSearchConfig struct {
	Dimensions, PopulationSize, Generations, TournamentSize int
	CrossoverRate, MutationRate, MutationSigma              float64
	LowerBound, UpperBound                                  float64
	Seed                                                    uint32
}

func DefaultGeneticSearchConfig() GeneticSearchConfig {
	return GeneticSearchConfig{2, 40, 80, 3, 0.85, 0.12, 0.20, -10, 10, 42}
}

type SimulatedAnnealingConfig struct {
	Dimensions, IterationsPerTemperature                int
	InitialTemperature, CoolingRate, MinimumTemperature float64
	StepSigma, LowerBound, UpperBound                   float64
	Seed                                                uint32
}

func DefaultSimulatedAnnealingConfig() SimulatedAnnealingConfig {
	return SimulatedAnnealingConfig{2, 40, 3, 0.95, 1e-4, 0.3, -10, 10, 123}
}

type HillClimbingConfig struct {
	Dimensions, MaxIterations, NeighboursPerIteration, MaxRestarts int
	StepSigma, LowerBound, UpperBound                              float64
	Seed                                                           uint32
}

func DefaultHillClimbingConfig() HillClimbingConfig {
	return HillClimbingConfig{2, 200, 24, 4, 0.25, -10, 10, 2026}
}

type QuantumSearchPolicy int

const (
	QuantumStandard QuantumSearchPolicy = iota
	QuantumCrop
	QuantumCropAndExpand
	QuantumWarp
)

type QuantumControlConfig struct {
	Policy                                                        QuantumSearchPolicy
	FrequencyCenter, InitialFrequencyRatio, CurrentFrequencyRatio float64
	AmplitudeMin, AmplitudeMax                                    float64
	InitialSamples, Iterations, AcquisitionCandidates             int
	ExplorationBeta, LengthScaleFrequency, LengthScaleAmplitude   float64
	ObservationNoise, CropMinRatio, ExpansionRate                 float64
	Seed                                                          uint32
}

func DefaultQuantumControlConfig() QuantumControlConfig {
	return QuantumControlConfig{QuantumWarp, 1, 0.5, 0.5, 0, 0.15, 8, 40, 128, 1.5, 0.15, 0.05, 1e-6, 0.01, 0.2, 431}
}

type SearchOptimizer struct {
	handle unsafe.Pointer
	kind   SearchOptimizerType
}

func NewSearchOptimizer(kind SearchOptimizerType, configuration any) (*SearchOptimizer, error) {
	var config []float64
	switch kind {
	case GeneticSearchOptimizer:
		c, ok := configuration.(GeneticSearchConfig)
		if !ok {
			return nil, errors.New("GeneticSearchOptimizer requires GeneticSearchConfig")
		}
		config = []float64{float64(c.Dimensions), float64(c.PopulationSize), float64(c.Generations),
			float64(c.TournamentSize), c.CrossoverRate, c.MutationRate, c.MutationSigma,
			c.LowerBound, c.UpperBound, float64(c.Seed)}
	case SimulatedAnnealingOptimizer:
		c, ok := configuration.(SimulatedAnnealingConfig)
		if !ok {
			return nil, errors.New("SimulatedAnnealingOptimizer requires SimulatedAnnealingConfig")
		}
		config = []float64{float64(c.Dimensions), float64(c.IterationsPerTemperature),
			c.InitialTemperature, c.CoolingRate, c.MinimumTemperature, c.StepSigma,
			c.LowerBound, c.UpperBound, float64(c.Seed)}
	case HillClimbingOptimizer:
		c, ok := configuration.(HillClimbingConfig)
		if !ok {
			return nil, errors.New("HillClimbingOptimizer requires HillClimbingConfig")
		}
		config = []float64{float64(c.Dimensions), float64(c.MaxIterations),
			float64(c.NeighboursPerIteration), float64(c.MaxRestarts), c.StepSigma,
			c.LowerBound, c.UpperBound, float64(c.Seed)}
	case QuantumControlBayesianOptimizer:
		c, ok := configuration.(QuantumControlConfig)
		if !ok {
			return nil, errors.New("QuantumControlBayesianOptimizer requires QuantumControlConfig")
		}
		config = []float64{float64(c.Policy), c.FrequencyCenter, c.InitialFrequencyRatio,
			c.CurrentFrequencyRatio, c.AmplitudeMin, c.AmplitudeMax,
			float64(c.InitialSamples), float64(c.Iterations), float64(c.AcquisitionCandidates),
			c.ExplorationBeta, c.LengthScaleFrequency, c.LengthScaleAmplitude,
			c.ObservationNoise, c.CropMinRatio, c.ExpansionRate, float64(c.Seed)}
	default:
		return nil, errors.New("unknown optimizer type")
	}
	var nativeErr [optimizationErrorCapacity]C.char
	var configPtr *C.double
	if len(config) != 0 {
		configPtr = (*C.double)(unsafe.Pointer(&config[0]))
	}
	handle := C.coolbox_optimizer_create(C.int(kind), configPtr, C.size_t(len(config)),
		&nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, optimizationNativeError(&nativeErr)
	}
	return &SearchOptimizer{handle: handle, kind: kind}, nil
}

type objectiveContext struct {
	fn  func([]float64) float64
	err error
}

//export coolboxGoObjective
func coolboxGoObjective(userData C.uintptr_t, values *C.double, count C.size_t) (result C.double) {
	context := cgo.Handle(userData).Value().(*objectiveContext)
	defer func() {
		if recovered := recover(); recovered != nil {
			context.err = fmt.Errorf("objective callback panicked: %v", recovered)
			result = C.double(math.NaN())
		}
	}()
	points := unsafe.Slice((*float64)(unsafe.Pointer(values)), int(count))
	return C.double(context.fn(points))
}

func (o *SearchOptimizer) Optimize(objective func([]float64) float64, initial []float64) ([]float64, error) {
	if o == nil || o.handle == nil {
		return nil, errors.New("optimizer is closed")
	}
	if objective == nil {
		return nil, errors.New("objective callback must not be nil")
	}
	context := &objectiveContext{fn: objective}
	handle := cgo.NewHandle(context)
	defer handle.Delete()
	var initialPtr *C.double
	if len(initial) != 0 {
		initialPtr = (*C.double)(unsafe.Pointer(&initial[0]))
	}
	var nativeErr [optimizationErrorCapacity]C.char
	if err := callNative(C.coolbox_optimizer_run(o.handle, C.coolbox_go_objective_callback(),
		C.uintptr_t(handle), initialPtr, C.size_t(len(initial)),
		&nativeErr[0], C.size_t(len(nativeErr))), &nativeErr); err != nil {
		return nil, err
	}
	if context.err != nil {
		return nil, context.err
	}
	size := int(C.coolbox_optimizer_solution_size(o.handle))
	result := make([]float64, size)
	var resultPtr *C.double
	if size != 0 {
		resultPtr = (*C.double)(unsafe.Pointer(&result[0]))
	}
	if err := callNative(C.coolbox_optimizer_copy_solution(o.handle, resultPtr, C.size_t(size),
		&nativeErr[0], C.size_t(len(nativeErr))), &nativeErr); err != nil {
		return nil, err
	}
	return result, nil
}

func (o *SearchOptimizer) BestScore() (float64, error) {
	if o == nil || o.handle == nil {
		return 0, errors.New("optimizer is closed")
	}
	return float64(C.coolbox_optimizer_best_score(o.handle)), nil
}

func (o *SearchOptimizer) BestProbability() (float64, error) {
	if o == nil || o.handle == nil {
		return 0, errors.New("optimizer is closed")
	}
	if o.kind != QuantumControlBayesianOptimizer {
		return 0, errors.New("best probability is available only for quantum-control optimization")
	}
	return float64(C.coolbox_optimizer_best_probability(o.handle)), nil
}

func (o *SearchOptimizer) QuantumBounds() ([5]float64, error) {
	var result [5]float64
	if o == nil || o.handle == nil {
		return result, errors.New("optimizer is closed")
	}
	var nativeErr [optimizationErrorCapacity]C.char
	err := callNative(C.coolbox_optimizer_quantum_bounds(o.handle, (*C.double)(unsafe.Pointer(&result[0])),
		C.size_t(len(result)), &nativeErr[0], C.size_t(len(nativeErr))), &nativeErr)
	return result, err
}

func (o *SearchOptimizer) TransformFrequencyPoint(point []float64, unwarp bool) ([]float64, error) {
	if o == nil || o.handle == nil {
		return nil, errors.New("optimizer is closed")
	}
	if o.kind != QuantumControlBayesianOptimizer {
		return nil, errors.New("frequency warping is available only for quantum-control optimization")
	}
	if len(point) == 0 {
		return nil, errors.New("point must not be empty")
	}
	result := make([]float64, len(point))
	var inPtr *C.double
	if len(point) > 0 {
		inPtr = (*C.double)(unsafe.Pointer(&point[0]))
	}
	var nativeErr [optimizationErrorCapacity]C.char
	flag := C.int(0)
	if unwarp {
		flag = 1
	}
	err := callNative(C.coolbox_optimizer_warp_point(o.handle, inPtr, C.size_t(len(point)),
		(*C.double)(unsafe.Pointer(&result[0])), flag, &nativeErr[0],
		C.size_t(len(nativeErr))), &nativeErr)
	return result, err
}

func (o *SearchOptimizer) Close() error {
	if o != nil && o.handle != nil {
		C.coolbox_optimizer_free(o.handle)
		o.handle = nil
	}
	return nil
}

type HeatGrid struct {
	Values     []float64
	NX, NY, NZ int
}

func SolveHeatEquation(initial HeatGrid, dimensions int, alpha, sigma, dx, dy, dz, dt float64, steps int, seed uint32, stochastic bool) (HeatGrid, error) {
	maxInt := int(^uint(0) >> 1)
	if initial.NX <= 0 || initial.NY <= 0 || initial.NZ <= 0 ||
		initial.NX > maxInt/initial.NY || initial.NX*initial.NY > maxInt/initial.NZ ||
		len(initial.Values) != initial.NX*initial.NY*initial.NZ {
		return HeatGrid{}, errors.New("heat grid shape does not match the supplied values")
	}
	result := HeatGrid{NX: initial.NX, NY: initial.NY, NZ: initial.NZ,
		Values: make([]float64, len(initial.Values))}
	if len(initial.Values) == 0 {
		return HeatGrid{}, errors.New("heat grid must not be empty")
	}
	stoch := C.int(0)
	if stochastic {
		stoch = 1
	}
	var nativeErr [optimizationErrorCapacity]C.char
	err := callNative(C.coolbox_math_heat(C.int(dimensions), stoch,
		(*C.double)(unsafe.Pointer(&initial.Values[0])), C.size_t(initial.NX),
		C.size_t(initial.NY), C.size_t(initial.NZ), C.double(alpha), C.double(sigma),
		C.double(dx), C.double(dy), C.double(dz), C.double(dt), C.int(steps),
		C.uint(seed), (*C.double)(unsafe.Pointer(&result.Values[0])),
		&nativeErr[0], C.size_t(len(nativeErr))), &nativeErr)
	return result, err
}

type FinanceFormula int

const (
	NormalPDF FinanceFormula = iota
	NormalCDF
	BlackScholesD1
	BlackScholesD2
	BlackScholesCallPrice
	BlackScholesPutPrice
	BlackScholesCallDelta
	BlackScholesPutDelta
	BlackScholesGamma
	BlackScholesVega
	BlackScholesCallTheta
	BlackScholesPutTheta
	BlackScholesCallRho
	BlackScholesPutRho
	DigitalCallPrice
	DigitalPutPrice
	GeometricAsianCallPrice
	GeometricAsianPutPrice
	DownAndInCallPrice
	DownAndOutCallPrice
	UpAndInCallPrice
	UpAndOutCallPrice
	DownAndInPutPrice
	DownAndOutPutPrice
)

type MonteCarloContract int

const (
	EuropeanCallContract MonteCarloContract = iota
	EuropeanPutContract
	ArithmeticAsianCallContract
	DownAndOutCallContract
	UpAndOutCallContract
	DownAndOutPutContract
)

type MonteCarloGreekContract int

const (
	EuropeanCallGreeks MonteCarloGreekContract = iota
	ArithmeticAsianCallGreeks
	DownAndOutCallGreeks
	UpAndOutCallGreeks
	DownAndOutPutGreeks
)

func EvaluateFinanceFormula(formula FinanceFormula, values ...float64) (float64, error) {
	var valuesPtr *C.double
	if len(values) != 0 {
		valuesPtr = (*C.double)(unsafe.Pointer(&values[0]))
	}
	var nativeErr [optimizationErrorCapacity]C.char
	result := C.coolbox_finance_formula(C.int(formula), valuesPtr, C.size_t(len(values)),
		&nativeErr[0], C.size_t(len(nativeErr)))
	if err := C.GoString(&nativeErr[0]); err != "" {
		return 0, errors.New(err)
	}
	return float64(result), nil
}

func BrownianMotionPath(times, shocks []float64, initial float64) ([]float64, error) {
	return financePath(false, initial, 0, 0, times, shocks)
}

func GeometricBrownianMotionPath(initial, drift, volatility float64, times, shocks []float64) ([]float64, error) {
	return financePath(true, initial, drift, volatility, times, shocks)
}

func financePath(geometric bool, initial, drift, volatility float64, times, shocks []float64) ([]float64, error) {
	if len(times) == 0 || len(times) != len(shocks) {
		return nil, errors.New("time steps and normal shocks must have equal, non-zero lengths")
	}
	result := make([]float64, len(times)+1)
	flag := C.int(0)
	if geometric {
		flag = 1
	}
	var nativeErr [optimizationErrorCapacity]C.char
	err := callNative(C.coolbox_finance_path(flag, C.double(initial), C.double(drift),
		C.double(volatility), (*C.double)(unsafe.Pointer(&times[0])), C.size_t(len(times)),
		(*C.double)(unsafe.Pointer(&shocks[0])), (*C.double)(unsafe.Pointer(&result[0])),
		&nativeErr[0], C.size_t(len(nativeErr))), &nativeErr)
	return result, err
}

func ItoIntegral(integrand, increments []float64) (float64, error) {
	return itoValue(false, integrand, increments)
}

func ItoIsometryVariance(integrand, timeSteps []float64) (float64, error) {
	return itoValue(true, integrand, timeSteps)
}

func itoValue(variance bool, values, increments []float64) (float64, error) {
	if len(values) == 0 || len(values) != len(increments) {
		return 0, errors.New("integrand and increments must have equal, non-zero lengths")
	}
	var result C.double
	flag := C.int(0)
	if variance {
		flag = 1
	}
	var nativeErr [optimizationErrorCapacity]C.char
	err := callNative(C.coolbox_finance_ito(flag, (*C.double)(unsafe.Pointer(&values[0])),
		(*C.double)(unsafe.Pointer(&increments[0])), C.size_t(len(values)), &result,
		&nativeErr[0], C.size_t(len(nativeErr))), &nativeErr)
	return float64(result), err
}

type MonteCarloEstimate struct {
	Price, StandardError, ConfidenceIntervalLow, ConfidenceIntervalHigh float64
	SampleCount                                                         uint
}

func EstimateMonteCarlo(contract MonteCarloContract, spot, strike, barrier, rate, volatility float64,
	times []float64, normalShockPaths [][]float64, confidenceZ float64) (MonteCarloEstimate, error) {
	if len(times) == 0 || len(normalShockPaths) == 0 {
		return MonteCarloEstimate{}, errors.New("time steps and shock paths must not be empty")
	}
	if len(normalShockPaths) > int(^uint(0)>>1)/len(times) {
		return MonteCarloEstimate{}, errors.New("normal shock dimensions exceed the native buffer limit")
	}
	flat := make([]float64, 0, len(times)*len(normalShockPaths))
	for _, path := range normalShockPaths {
		if len(path) != len(times) {
			return MonteCarloEstimate{}, errors.New("each normal shock path must match the number of time steps")
		}
		flat = append(flat, path...)
	}
	var native C.coolbox_estimate
	var nativeErr [optimizationErrorCapacity]C.char
	err := callNative(C.coolbox_finance_monte_carlo(C.int(contract), C.double(spot),
		C.double(strike), C.double(barrier), C.double(rate), C.double(volatility),
		(*C.double)(unsafe.Pointer(&times[0])), C.size_t(len(times)),
		(*C.double)(unsafe.Pointer(&flat[0])), C.size_t(len(normalShockPaths)),
		C.double(confidenceZ), C.double(0.01), C.double(0.01), &native,
		&nativeErr[0], C.size_t(len(nativeErr))), &nativeErr)
	return MonteCarloEstimate{float64(native.price), float64(native.standard_error),
		float64(native.confidence_interval_low), float64(native.confidence_interval_high),
		uint(native.sample_count)}, err
}

type MonteCarloGreeks struct {
	Delta, Gamma, Vega, Theta, Rho MonteCarloEstimate
}

func EstimateMonteCarloGreeks(contract MonteCarloGreekContract, spot, strike, barrier, rate, volatility float64,
	times []float64, normalShockPaths [][]float64, confidenceZ, relativeBump, timeBump float64) (MonteCarloGreeks, error) {
	if len(times) == 0 || len(normalShockPaths) == 0 {
		return MonteCarloGreeks{}, errors.New("time steps and shock paths must not be empty")
	}
	if len(normalShockPaths) > int(^uint(0)>>1)/len(times) {
		return MonteCarloGreeks{}, errors.New("normal shock dimensions exceed the native buffer limit")
	}
	flat := make([]float64, 0, len(times)*len(normalShockPaths))
	for _, path := range normalShockPaths {
		if len(path) != len(times) {
			return MonteCarloGreeks{}, errors.New("each normal shock path must match the number of time steps")
		}
		flat = append(flat, path...)
	}
	var native C.coolbox_greeks
	var nativeErr [optimizationErrorCapacity]C.char
	err := callNative(C.coolbox_finance_monte_carlo_greeks(C.int(contract),
		C.double(spot), C.double(strike), C.double(barrier), C.double(rate),
		C.double(volatility), (*C.double)(unsafe.Pointer(&times[0])),
		C.size_t(len(times)), (*C.double)(unsafe.Pointer(&flat[0])),
		C.size_t(len(normalShockPaths)), C.double(confidenceZ), C.double(relativeBump),
		C.double(timeBump), &native, &nativeErr[0], C.size_t(len(nativeErr))), &nativeErr)
	estimate := func(value C.coolbox_estimate) MonteCarloEstimate {
		return MonteCarloEstimate{float64(value.price), float64(value.standard_error),
			float64(value.confidence_interval_low), float64(value.confidence_interval_high),
			uint(value.sample_count)}
	}
	return MonteCarloGreeks{estimate(native.delta), estimate(native.gamma),
		estimate(native.vega), estimate(native.theta), estimate(native.rho)}, err
}

type MonteCarloPayoff func(path []float64) (float64, error)

type payoffContext struct {
	fn  MonteCarloPayoff
	err error
}

//export coolboxGoPayoff
func coolboxGoPayoff(userData C.uintptr_t, path *C.double, count C.size_t, result *C.double) (ok C.int) {
	context := cgo.Handle(userData).Value().(*payoffContext)
	defer func() {
		if recovered := recover(); recovered != nil {
			context.err = fmt.Errorf("Monte Carlo payoff callback panicked: %v", recovered)
			ok = 0
		}
	}()
	values := append([]float64(nil), unsafe.Slice((*float64)(unsafe.Pointer(path)), int(count))...)
	payoff, err := context.fn(values)
	if err != nil {
		context.err = err
		return 0
	}
	*result = C.double(payoff)
	return 1
}

func EstimateMonteCarloCustomPayoff(spot, rate, volatility float64, times []float64,
	normalShockPaths [][]float64, confidenceZ float64, payoff MonteCarloPayoff) (MonteCarloEstimate, error) {
	if payoff == nil {
		return MonteCarloEstimate{}, errors.New("custom payoff callback must not be nil")
	}
	flat, err := flattenShockPaths(times, normalShockPaths)
	if err != nil {
		return MonteCarloEstimate{}, err
	}
	context := &payoffContext{fn: payoff}
	handle := cgo.NewHandle(context)
	defer handle.Delete()
	var native C.coolbox_estimate
	var nativeErr [optimizationErrorCapacity]C.char
	ok := C.coolbox_finance_monte_carlo_generic(C.double(spot), C.double(rate),
		C.double(volatility), (*C.double)(unsafe.Pointer(&times[0])),
		C.size_t(len(times)), (*C.double)(unsafe.Pointer(&flat[0])),
		C.size_t(len(normalShockPaths)), C.double(confidenceZ),
		C.coolbox_go_payoff_callback(), C.uintptr_t(handle), &native,
		&nativeErr[0], C.size_t(len(nativeErr)))
	if err := callNative(ok, &nativeErr); err != nil {
		if context.err != nil {
			return MonteCarloEstimate{}, context.err
		}
		return MonteCarloEstimate{}, err
	}
	return monteCarloEstimateFromNative(native), nil
}

func EstimateMonteCarloCustomPayoffGreeks(spot, strike, rate, volatility float64,
	times []float64, normalShockPaths [][]float64, confidenceZ, relativeBump, timeBump float64,
	payoff MonteCarloPayoff) (MonteCarloGreeks, error) {
	if payoff == nil {
		return MonteCarloGreeks{}, errors.New("custom payoff callback must not be nil")
	}
	flat, err := flattenShockPaths(times, normalShockPaths)
	if err != nil {
		return MonteCarloGreeks{}, err
	}
	context := &payoffContext{fn: payoff}
	handle := cgo.NewHandle(context)
	defer handle.Delete()
	var native C.coolbox_greeks
	var nativeErr [optimizationErrorCapacity]C.char
	ok := C.coolbox_finance_monte_carlo_greeks_generic(
		C.double(spot), C.double(strike), C.double(rate), C.double(volatility),
		(*C.double)(unsafe.Pointer(&times[0])), C.size_t(len(times)),
		(*C.double)(unsafe.Pointer(&flat[0])), C.size_t(len(normalShockPaths)),
		C.double(confidenceZ), C.double(relativeBump), C.double(timeBump),
		C.coolbox_go_payoff_callback(), C.uintptr_t(handle), &native,
		&nativeErr[0], C.size_t(len(nativeErr)))
	if err := callNative(ok, &nativeErr); err != nil {
		if context.err != nil {
			return MonteCarloGreeks{}, context.err
		}
		return MonteCarloGreeks{}, err
	}
	return MonteCarloGreeks{monteCarloEstimateFromNative(native.delta),
		monteCarloEstimateFromNative(native.gamma), monteCarloEstimateFromNative(native.vega),
		monteCarloEstimateFromNative(native.theta), monteCarloEstimateFromNative(native.rho)}, nil
}

func flattenShockPaths(times []float64, normalShockPaths [][]float64) ([]float64, error) {
	if len(times) == 0 || len(normalShockPaths) == 0 {
		return nil, errors.New("time steps and shock paths must not be empty")
	}
	if len(normalShockPaths) > int(^uint(0)>>1)/len(times) {
		return nil, errors.New("normal shock dimensions exceed the native buffer limit")
	}
	flat := make([]float64, 0, len(times)*len(normalShockPaths))
	for _, path := range normalShockPaths {
		if len(path) != len(times) {
			return nil, errors.New("each normal shock path must match the number of time steps")
		}
		flat = append(flat, path...)
	}
	return flat, nil
}

func monteCarloEstimateFromNative(value C.coolbox_estimate) MonteCarloEstimate {
	return MonteCarloEstimate{float64(value.price), float64(value.standard_error),
		float64(value.confidence_interval_low), float64(value.confidence_interval_high),
		uint(value.sample_count)}
}

func NaiveAverageParameters(models [][]float64) ([]float64, error) {
	if len(models) == 0 || len(models[0]) == 0 {
		return nil, errors.New("parameter models must not be empty")
	}
	width := len(models[0])
	flat := make([]float64, 0, len(models)*width)
	for _, model := range models {
		if len(model) != width {
			return nil, errors.New("all parameter models must have equal lengths")
		}
		flat = append(flat, model...)
	}
	output := make([]float64, width)
	var nativeErr [optimizationErrorCapacity]C.char
	err := callNative(C.coolbox_owa_naive_average((*C.double)(unsafe.Pointer(&flat[0])),
		C.size_t(len(models)), C.size_t(width), (*C.double)(unsafe.Pointer(&output[0])),
		&nativeErr[0], C.size_t(len(nativeErr))), &nativeErr)
	return output, err
}

func OptimalWeightedAverageLinearRegression(models, features [][]float64, targets []float64,
	iterations int, learningRate, regularization float64, projectToSimplex bool) ([]float64, []float64, float64, error) {
	if len(models) == 0 || len(models[0]) == 0 || len(features) == 0 || len(features[0]) == 0 ||
		len(targets) != len(features) {
		return nil, nil, 0, errors.New("model parameters and validation data must be non-empty and have matching row counts")
	}
	parameterWidth, featureWidth := len(models[0]), len(features[0])
	if parameterWidth != featureWidth {
		return nil, nil, 0, errors.New("model parameter and validation feature widths must match")
	}
	parametersFlat := make([]float64, 0, len(models)*parameterWidth)
	for _, model := range models {
		if len(model) != parameterWidth {
			return nil, nil, 0, errors.New("all model parameter lengths must match")
		}
		parametersFlat = append(parametersFlat, model...)
	}
	featuresFlat := make([]float64, 0, len(features)*featureWidth)
	for _, row := range features {
		if len(row) != featureWidth {
			return nil, nil, 0, errors.New("all validation feature rows must have equal lengths")
		}
		featuresFlat = append(featuresFlat, row...)
	}
	merged, weights := make([]float64, parameterWidth), make([]float64, len(models))
	var loss C.double
	project := C.int(0)
	if projectToSimplex {
		project = 1
	}
	var nativeErr [optimizationErrorCapacity]C.char
	err := callNative(C.coolbox_owa_linear_regression(
		(*C.double)(unsafe.Pointer(&parametersFlat[0])), C.size_t(len(models)),
		C.size_t(parameterWidth), (*C.double)(unsafe.Pointer(&featuresFlat[0])),
		C.size_t(len(features)), C.size_t(featureWidth),
		(*C.double)(unsafe.Pointer(&targets[0])), C.size_t(len(targets)),
		C.int(iterations), C.double(learningRate), C.double(regularization), project,
		(*C.double)(unsafe.Pointer(&merged[0])), (*C.double)(unsafe.Pointer(&weights[0])),
		&loss, &nativeErr[0], C.size_t(len(nativeErr))), &nativeErr)
	return merged, weights, float64(loss), err
}

type OwaConfig struct {
	Iterations       int
	LearningRate     float64
	Regularization   float64
	ProjectToSimplex bool
}

func DefaultOwaConfig() OwaConfig {
	return OwaConfig{Iterations: 500, LearningRate: 0.05, Regularization: 1e-4, ProjectToSimplex: true}
}

type OwaFold struct {
	Features [][]float64
	Targets  []float64
}

type OwaCrossValidationResult struct {
	MeanLoss   float64
	FoldLosses []float64
}

func FastOwaCrossValidationLinearRegression(models [][]float64, folds []OwaFold,
	config OwaConfig) (OwaCrossValidationResult, error) {
	if len(models) < 2 || len(folds) != len(models) || len(models[0]) == 0 {
		return OwaCrossValidationResult{}, errors.New("OWA cross-validation requires at least two models and one fold per model")
	}
	parameterCount := len(models[0])
	featureCount := parameterCount
	parameters := make([]float64, 0, len(models)*parameterCount)
	for _, model := range models {
		if len(model) != parameterCount {
			return OwaCrossValidationResult{}, errors.New("all model parameter lengths must match")
		}
		parameters = append(parameters, model...)
	}
	rowOffsets := make([]C.size_t, len(folds)+1)
	targetOffsets := make([]C.size_t, len(folds)+1)
	featuresFlat := make([]float64, 0)
	targetsFlat := make([]float64, 0)
	for i, fold := range folds {
		if len(fold.Features) == 0 || len(fold.Targets) != len(fold.Features) {
			return OwaCrossValidationResult{}, fmt.Errorf("fold %d must have non-empty aligned features and targets", i)
		}
		for _, row := range fold.Features {
			if len(row) != featureCount {
				return OwaCrossValidationResult{}, fmt.Errorf("fold %d feature width does not match model parameters", i)
			}
			featuresFlat = append(featuresFlat, row...)
		}
		targetsFlat = append(targetsFlat, fold.Targets...)
		rowOffsets[i+1] = C.size_t(len(featuresFlat) / featureCount)
		targetOffsets[i+1] = C.size_t(len(targetsFlat))
	}
	if config.Iterations < 0 || uint64(config.Iterations) > uint64(^uint32(0)>>1) {
		return OwaCrossValidationResult{}, errors.New("OWA iterations must fit a non-negative native integer")
	}
	foldLosses := make([]float64, len(folds))
	var meanLoss C.double
	var nativeErr [optimizationErrorCapacity]C.char
	project := C.int(0)
	if config.ProjectToSimplex {
		project = 1
	}
	ok := C.coolbox_owa_cross_validation_linear_regression(
		(*C.double)(unsafe.Pointer(&parameters[0])), C.size_t(len(models)),
		C.size_t(parameterCount), (*C.double)(unsafe.Pointer(&featuresFlat[0])),
		&rowOffsets[0], (*C.double)(unsafe.Pointer(&targetsFlat[0])),
		&targetOffsets[0], C.size_t(len(folds)), C.size_t(featureCount),
		C.int(config.Iterations), C.double(config.LearningRate),
		C.double(config.Regularization), project,
		(*C.double)(unsafe.Pointer(&foldLosses[0])), &meanLoss,
		&nativeErr[0], C.size_t(len(nativeErr)))
	if err := callNative(ok, &nativeErr); err != nil {
		return OwaCrossValidationResult{}, err
	}
	return OwaCrossValidationResult{float64(meanLoss), foldLosses}, nil
}

type TensorData struct {
	Shape  []int
	Values []float64
}

type NowaLoss func(layers []TensorData) (float64, error)

type nowaLossContext struct {
	shapes     [][]int
	layerSizes []int
	fn         NowaLoss
	err        error
}

//export coolboxGoNowaLoss
func coolboxGoNowaLoss(userData C.uintptr_t, values *C.double, layerSizes *C.size_t,
	layerCount C.size_t, result *C.double) (ok C.int) {
	context := cgo.Handle(userData).Value().(*nowaLossContext)
	defer func() {
		if recovered := recover(); recovered != nil {
			context.err = fmt.Errorf("NOWA validation callback panicked: %v", recovered)
			ok = 0
		}
	}()
	if int(layerCount) != len(context.shapes) {
		context.err = errors.New("NOWA callback received an unexpected layer count")
		return 0
	}
	sizes := unsafe.Slice(layerSizes, int(layerCount))
	total := 0
	for i, size := range sizes {
		if uint64(size) != uint64(context.layerSizes[i]) {
			context.err = errors.New("NOWA callback received an unexpected layer size")
			return 0
		}
		if context.layerSizes[i] > int(^uint(0)>>1)-total {
			context.err = errors.New("NOWA callback tensor data size overflows")
			return 0
		}
		total += context.layerSizes[i]
	}
	flat := unsafe.Slice((*float64)(unsafe.Pointer(values)), total)
	layers := make([]TensorData, int(layerCount))
	offset := 0
	for i, size := range context.layerSizes {
		end := offset + size
		layers[i] = TensorData{Shape: append([]int(nil), context.shapes[i]...),
			Values: append([]float64(nil), flat[offset:end]...)}
		offset = end
	}
	loss, err := context.fn(layers)
	if err != nil {
		context.err = err
		return 0
	}
	*result = C.double(loss)
	return 1
}

type NowaResult struct {
	MergedLayers   []*Tensor
	LayerWeights   [][]float64
	ValidationLoss float64
}

func NonlinearOptimalWeightedAverageLayers(models [][]*Tensor, loss NowaLoss,
	config OwaConfig) (NowaResult, error) {
	if len(models) == 0 || len(models[0]) == 0 || loss == nil {
		return NowaResult{}, errors.New("NOWA requires non-empty model layers and a validation callback")
	}
	layerCount := len(models[0])
	shapes := make([][]int, layerCount)
	shapeOffsets := make([]C.size_t, layerCount+1)
	valueOffsets := make([]C.size_t, layerCount+1)
	layerSizes := make([]C.size_t, layerCount)
	layerDimensions := make([]C.size_t, layerCount)
	totalPerModel := 0
	for modelIndex, model := range models {
		if len(model) != layerCount {
			return NowaResult{}, fmt.Errorf("model %d has a different layer count", modelIndex)
		}
		for layerIndex, tensor := range model {
			if tensor == nil {
				return NowaResult{}, fmt.Errorf("model %d layer %d is nil", modelIndex, layerIndex)
			}
			shape, err := tensor.Shape()
			if err != nil {
				return NowaResult{}, err
			}
			if modelIndex == 0 {
				values, err := tensor.Values()
				if err != nil {
					return NowaResult{}, err
				}
				if len(values) > int(^uint(0)>>1)-totalPerModel {
					return NowaResult{}, errors.New("NOWA tensor dimensions exceed the native buffer limit")
				}
				shapes[layerIndex] = shape
				layerSizes[layerIndex] = C.size_t(len(values))
				layerDimensions[layerIndex] = C.size_t(len(shape))
				shapeOffsets[layerIndex+1] = shapeOffsets[layerIndex] + C.size_t(len(shape))
				totalPerModel += len(values)
				valueOffsets[layerIndex+1] = C.size_t(totalPerModel)
			} else if !equalIntSlices(shape, shapes[layerIndex]) {
				return NowaResult{}, fmt.Errorf("model %d layer %d has a different tensor shape", modelIndex, layerIndex)
			}
		}
	}
	if totalPerModel <= 0 || len(models) > int(^uint(0)>>1)/totalPerModel ||
		len(models) > int(^uint(0)>>1)/layerCount {
		return NowaResult{}, errors.New("NOWA tensor dimensions exceed the native buffer limit")
	}
	localFlat := make([]float64, 0, len(models)*totalPerModel)
	for _, model := range models {
		for _, tensor := range model {
			values, err := tensor.Values()
			if err != nil {
				return NowaResult{}, err
			}
			localFlat = append(localFlat, values...)
		}
	}
	if uint64(shapeOffsets[layerCount]) > uint64(int(^uint(0)>>1)) {
		return NowaResult{}, errors.New("NOWA shape metadata exceeds the native buffer limit")
	}
	shapeFlat := make([]C.size_t, 0, int(shapeOffsets[layerCount]))
	for _, shape := range shapes {
		for _, dimension := range shape {
			shapeFlat = append(shapeFlat, C.size_t(dimension))
		}
	}
	mergedFlat := make([]float64, totalPerModel)
	weightsFlat := make([]float64, len(models)*layerCount)
	sizesInGo := make([]int, len(layerSizes))
	for i, size := range layerSizes {
		sizesInGo[i] = int(size)
	}
	context := &nowaLossContext{shapes: shapes, layerSizes: sizesInGo, fn: loss}
	handle := cgo.NewHandle(context)
	defer handle.Delete()
	if config.Iterations < 0 || uint64(config.Iterations) > uint64(^uint32(0)>>1) {
		return NowaResult{}, errors.New("OWA iterations must fit a non-negative native integer")
	}
	project := C.int(0)
	if config.ProjectToSimplex {
		project = 1
	}
	var validationLoss C.double
	var nativeErr [optimizationErrorCapacity]C.char
	ok := C.coolbox_owa_nonlinear_layers(
		(*C.double)(unsafe.Pointer(&localFlat[0])), C.size_t(len(models)),
		C.size_t(layerCount), (*C.size_t)(unsafe.Pointer(&shapeFlat[0])),
		(*C.size_t)(unsafe.Pointer(&shapeOffsets[0])),
		(*C.size_t)(unsafe.Pointer(&layerSizes[0])),
		(*C.size_t)(unsafe.Pointer(&valueOffsets[0])),
		(*C.size_t)(unsafe.Pointer(&layerDimensions[0])), C.int(config.Iterations),
		C.double(config.LearningRate), C.double(config.Regularization), project,
		C.coolbox_go_nowa_loss_callback(), C.uintptr_t(handle),
		(*C.double)(unsafe.Pointer(&mergedFlat[0])),
		(*C.double)(unsafe.Pointer(&weightsFlat[0])), &validationLoss,
		&nativeErr[0], C.size_t(len(nativeErr)))
	if err := callNative(ok, &nativeErr); err != nil {
		if context.err != nil {
			return NowaResult{}, context.err
		}
		return NowaResult{}, err
	}
	result := NowaResult{LayerWeights: make([][]float64, layerCount),
		ValidationLoss: float64(validationLoss)}
	mergedOffset := 0
	for layer, size := range layerSizes {
		count := int(size)
		tensor, err := NewTensorFromData(shapes[layer], mergedFlat[mergedOffset:mergedOffset+count])
		if err != nil {
			for _, allocated := range result.MergedLayers {
				_ = allocated.Close()
			}
			return NowaResult{}, err
		}
		result.MergedLayers = append(result.MergedLayers, tensor)
		mergedOffset += count
		begin := layer * len(models)
		result.LayerWeights[layer] = append([]float64(nil), weightsFlat[begin:begin+len(models)]...)
	}
	return result, nil
}

func WeightedTensorAverage(tensors []*Tensor, weights []float64) (*Tensor, error) {
	if len(tensors) == 0 || len(tensors) != len(weights) {
		return nil, errors.New("weighted tensor averaging requires one weight per tensor")
	}
	if tensors[0] == nil {
		return nil, errors.New("tensor must not be nil")
	}
	shape, err := tensors[0].Shape()
	if err != nil {
		return nil, err
	}
	width := 1
	for _, dimension := range shape {
		if dimension <= 0 || width > int(^uint(0)>>1)/dimension {
			return nil, errors.New("tensor dimensions are invalid or overflow")
		}
		width *= dimension
	}
	if len(tensors) > int(^uint(0)>>1)/width {
		return nil, errors.New("weighted tensor dimensions exceed the native buffer limit")
	}
	flat := make([]float64, 0, len(tensors)*width)
	for index, tensor := range tensors {
		if tensor == nil {
			return nil, fmt.Errorf("tensor %d is nil", index)
		}
		currentShape, err := tensor.Shape()
		if err != nil {
			return nil, err
		}
		if !equalIntSlices(currentShape, shape) {
			return nil, fmt.Errorf("tensor %d has a different shape", index)
		}
		values, err := tensor.Values()
		if err != nil {
			return nil, err
		}
		flat = append(flat, values...)
	}
	merged := make([]float64, width)
	shapeNative := make([]C.size_t, len(shape))
	for i, dimension := range shape {
		shapeNative[i] = C.size_t(dimension)
	}
	var nativeErr [optimizationErrorCapacity]C.char
	ok := C.coolbox_owa_weighted_tensor_average(
		(*C.double)(unsafe.Pointer(&flat[0])), C.size_t(len(tensors)),
		(*C.size_t)(unsafe.Pointer(&shapeNative[0])), C.size_t(len(shape)),
		(*C.double)(unsafe.Pointer(&weights[0])), C.size_t(width),
		(*C.double)(unsafe.Pointer(&merged[0])), &nativeErr[0],
		C.size_t(len(nativeErr)))
	if err := callNative(ok, &nativeErr); err != nil {
		return nil, err
	}
	return NewTensorFromData(shape, merged)
}

func equalIntSlices(left, right []int) bool {
	if len(left) != len(right) {
		return false
	}
	for i := range left {
		if left[i] != right[i] {
			return false
		}
	}
	return true
}
