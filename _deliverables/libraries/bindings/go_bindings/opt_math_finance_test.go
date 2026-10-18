package coolbox

import (
	"errors"
	"math"
	"testing"
)

func TestFinanceFormulaAndPaths(t *testing.T) {
	call, err := EvaluateFinanceFormula(BlackScholesCallPrice, 100, 100, 0.05, 0.2, 1)
	if err != nil {
		t.Fatal(err)
	}
	if math.Abs(call-10.4505835722) > 1e-6 {
		t.Fatalf("Black-Scholes call = %.10f; want 10.4505835722", call)
	}
	path, err := BrownianMotionPath([]float64{0.25, 0.25}, []float64{0, 2}, 0)
	if err != nil {
		t.Fatal(err)
	}
	if len(path) != 3 || path[2] != 1 {
		t.Fatalf("BrownianMotionPath = %v; want [0 0 1]", path)
	}
}

func TestHeatEquationAndOptimizer(t *testing.T) {
	grid, err := SolveHeatEquation(HeatGrid{
		Values: []float64{0, 1, 0},
		NX:     3, NY: 1, NZ: 1,
	}, 1, 1, 0, 1, 1, 1, 0.1, 1, 42, false)
	if err != nil {
		t.Fatal(err)
	}
	if grid.Values[1] <= 0 || grid.Values[1] >= 1 {
		t.Fatalf("heat solution center = %v; want a value in (0,1)", grid.Values[1])
	}

	config := DefaultHillClimbingConfig()
	config.Dimensions = 1
	config.MaxIterations = 40
	config.NeighboursPerIteration = 20
	config.MaxRestarts = 1
	optimizer, err := NewSearchOptimizer(HillClimbingOptimizer, config)
	if err != nil {
		t.Fatal(err)
	}
	defer optimizer.Close()
	solution, err := optimizer.Optimize(func(point []float64) float64 {
		return point[0] * point[0]
	}, []float64{4})
	if err != nil {
		t.Fatal(err)
	}
	score, err := optimizer.BestScore()
	if err != nil || len(solution) != 1 || score > 4 {
		t.Fatalf("Optimize() = %v, score %v, err %v", solution, score, err)
	}
}

func TestDistributedParameterMerging(t *testing.T) {
	mean, err := NaiveAverageParameters([][]float64{{1, 3}, {3, 5}})
	if err != nil || mean[0] != 2 || mean[1] != 4 {
		t.Fatalf("NaiveAverageParameters() = %v, %v; want [2 4]", mean, err)
	}
}

func TestExtendedMonteCarloAndOwaCallbacks(t *testing.T) {
	estimate, err := EstimateMonteCarloCustomPayoff(100, 0.05, 0.2,
		[]float64{1}, [][]float64{{0}}, 1.959963984540054,
		func(path []float64) (float64, error) {
			if len(path) != 2 {
				return 0, errors.New("payoff path must include initial and terminal values")
			}
			return path[1], nil
		})
	if err != nil {
		t.Fatal(err)
	}
	if estimate.SampleCount != 1 || math.IsNaN(estimate.Price) {
		t.Fatalf("custom payoff estimate = %+v", estimate)
	}
	greeks, err := EstimateMonteCarloCustomPayoffGreeks(100, 100, 0.05, 0.2,
		[]float64{1}, [][]float64{{0}}, 1.959963984540054, 0.01, 0.01,
		func(path []float64) (float64, error) { return path[len(path)-1], nil })
	if err != nil || math.IsNaN(greeks.Delta.Price) {
		t.Fatalf("custom payoff Greeks = %+v, %v", greeks, err)
	}
	_, err = EstimateMonteCarloCustomPayoff(100, 0.05, 0.2,
		[]float64{1}, [][]float64{{0}}, 1.959963984540054,
		func([]float64) (float64, error) {
			return 0, errors.New("payoff rejected path")
		})
	if err == nil || err.Error() != "payoff rejected path" {
		t.Fatalf("custom payoff error = %v; want callback error", err)
	}

	cv, err := FastOwaCrossValidationLinearRegression(
		[][]float64{{0, 1}, {2, 1}, {4, 3}},
		[]OwaFold{
			{Features: [][]float64{{1, 2}}, Targets: []float64{2}},
			{Features: [][]float64{{2, 1}}, Targets: []float64{1}},
			{Features: [][]float64{{1, 1}}, Targets: []float64{3}},
		},
		OwaConfig{Iterations: 10, LearningRate: 0.05, Regularization: 1e-4, ProjectToSimplex: true},
	)
	if err != nil || len(cv.FoldLosses) != 3 || math.IsNaN(cv.MeanLoss) {
		t.Fatalf("OWA cross-validation = %+v, %v", cv, err)
	}

	models := make([][]*Tensor, 2)
	models[0] = make([]*Tensor, 2)
	models[1] = make([]*Tensor, 2)
	models[0][0], err = NewTensorFromData([]int{2}, []float64{1, 2})
	if err != nil {
		t.Fatal(err)
	}
	models[0][1], err = NewTensorFromData([]int{1}, []float64{3})
	if err != nil {
		t.Fatal(err)
	}
	models[1][0], err = NewTensorFromData([]int{2}, []float64{3, 1})
	if err != nil {
		t.Fatal(err)
	}
	models[1][1], err = NewTensorFromData([]int{1}, []float64{5})
	if err != nil {
		t.Fatal(err)
	}
	for _, model := range models {
		for _, tensor := range model {
			defer tensor.Close()
		}
	}
	average, err := WeightedTensorAverage(
		[]*Tensor{models[0][0], models[1][0]}, []float64{0.5, 0.5})
	if err != nil {
		t.Fatal(err)
	}
	defer average.Close()
	averageValues, err := average.Values()
	if err != nil || len(averageValues) != 2 || averageValues[0] != 2 || averageValues[1] != 1.5 {
		t.Fatalf("WeightedTensorAverage() = %v, %v; want [2 1.5]", averageValues, err)
	}
	nowa, err := NonlinearOptimalWeightedAverageLayers(models,
		func(layers []TensorData) (float64, error) {
			loss := 0.0
			for _, layer := range layers {
				for _, value := range layer.Values {
					loss += value * value
				}
			}
			return loss, nil
		}, OwaConfig{Iterations: 10, LearningRate: 0.05, Regularization: 1e-4, ProjectToSimplex: true})
	if err != nil {
		t.Fatal(err)
	}
	for _, tensor := range nowa.MergedLayers {
		defer tensor.Close()
	}
	if len(nowa.MergedLayers) != 2 || len(nowa.LayerWeights) != 2 ||
		math.IsNaN(nowa.ValidationLoss) {
		t.Fatalf("NOWA result = %+v", nowa)
	}
	_, err = NonlinearOptimalWeightedAverageLayers(models,
		func([]TensorData) (float64, error) { return 0, errors.New("validation loss rejected layers") },
		OwaConfig{Iterations: 10, LearningRate: 0.05, Regularization: 1e-4, ProjectToSimplex: true})
	if err == nil || err.Error() != "validation loss rejected layers" {
		t.Fatalf("NOWA callback error = %v; want validation callback error", err)
	}
}
