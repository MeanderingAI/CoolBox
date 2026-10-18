package coolbox

import (
	"errors"
	"fmt"
	"math"
	"math/rand"
)

// KMeansInitMethod selects how the initial centroids are chosen.
type KMeansInitMethod int

const (
	KMeansRandom KMeansInitMethod = iota
	KMeansPlusPlus
)

// KMeansResult contains a snapshot of a fitted clustering result.
type KMeansResult struct {
	Centroids      [][]float64
	Labels         []int
	InertiaHistory []float64
	Inertia        float64
	Iterations     int
}

// KMeans implements the Python binding's Lloyd clustering algorithm in Go.
type KMeans struct {
	clusterCount  int
	maxIterations int
	initMethod    KMeansInitMethod
	randomState   int64
	result        KMeansResult
	fitted        bool
}

// NewKMeans constructs a KMeans model.
func NewKMeans(clusterCount, maxIterations int, init KMeansInitMethod, randomState int64) (*KMeans, error) {
	if clusterCount < 1 {
		return nil, errors.New("coolbox: number of clusters must be positive")
	}
	if maxIterations < 0 {
		return nil, errors.New("coolbox: max iterations cannot be negative")
	}
	if init != KMeansRandom && init != KMeansPlusPlus {
		return nil, fmt.Errorf("coolbox: invalid KMeans initialization method %d", init)
	}
	return &KMeans{
		clusterCount:  clusterCount,
		maxIterations: maxIterations,
		initMethod:    init,
		randomState:   randomState,
	}, nil
}

// DefaultKMeans creates the Python API's default configuration.
func DefaultKMeans() *KMeans {
	return &KMeans{
		clusterCount:  3,
		maxIterations: 300,
		initMethod:    KMeansPlusPlus,
		randomState:   42,
	}
}

// Fit clusters rows of features.
func (model *KMeans) Fit(features [][]float64) error {
	if model == nil {
		return errors.New("coolbox: KMeans model is nil")
	}
	rows, columns, err := validateFloatMatrix(features)
	if err != nil {
		return err
	}
	if rows < model.clusterCount {
		return errors.New("coolbox: number of samples must be at least the number of clusters")
	}

	random := rand.New(rand.NewSource(model.randomState))
	centroids := model.initialize(features, random)
	history := make([]float64, 0, model.maxIterations)
	iterations := 0
	for ; iterations < model.maxIterations; iterations++ {
		labels := assignClusters(features, centroids)
		history = append(history, clusterInertia(features, centroids, labels))
		next := updateCentroids(features, labels, model.clusterCount, columns)
		if centroidsClose(next, centroids) {
			centroids = next
			iterations++
			break
		}
		centroids = next
	}

	labels := assignClusters(features, centroids)
	model.result = KMeansResult{
		Centroids:      cloneMatrix(centroids),
		Labels:         labels,
		InertiaHistory: history,
		Inertia:        clusterInertia(features, centroids, labels),
		Iterations:     iterations,
	}
	model.fitted = true
	return nil
}

// FitPredict fits the model and returns a copy of the sample labels.
func (model *KMeans) FitPredict(features [][]float64) ([]int, error) {
	if err := model.Fit(features); err != nil {
		return nil, err
	}
	return model.Labels()
}

// Predict assigns each row to its nearest fitted centroid.
func (model *KMeans) Predict(features [][]float64) ([]int, error) {
	if err := model.checkFitted(); err != nil {
		return nil, err
	}
	_, columns, err := validateFloatMatrix(features)
	if err != nil {
		return nil, err
	}
	if columns != len(model.result.Centroids[0]) {
		return nil, errors.New("coolbox: feature count does not match fitted KMeans model")
	}
	return assignClusters(features, model.result.Centroids), nil
}

// Result returns defensive copies of the fitted clustering results.
func (model *KMeans) Result() (KMeansResult, error) {
	if err := model.checkFitted(); err != nil {
		return KMeansResult{}, err
	}
	return KMeansResult{
		Centroids:      cloneMatrix(model.result.Centroids),
		Labels:         append([]int(nil), model.result.Labels...),
		InertiaHistory: append([]float64(nil), model.result.InertiaHistory...),
		Inertia:        model.result.Inertia,
		Iterations:     model.result.Iterations,
	}, nil
}

// Centroids returns the fitted cluster centers.
func (model *KMeans) Centroids() ([][]float64, error) {
	result, err := model.Result()
	return result.Centroids, err
}

// Labels returns the fitted labels for the training observations.
func (model *KMeans) Labels() ([]int, error) {
	result, err := model.Result()
	return result.Labels, err
}

// Inertia returns the fitted within-cluster sum of squared errors.
func (model *KMeans) Inertia() (float64, error) {
	result, err := model.Result()
	return result.Inertia, err
}

// InertiaHistory returns the within-cluster error recorded after each iteration.
func (model *KMeans) InertiaHistory() ([]float64, error) {
	result, err := model.Result()
	return result.InertiaHistory, err
}

// Iterations returns the number of Lloyd iterations performed.
func (model *KMeans) Iterations() (int, error) {
	result, err := model.Result()
	return result.Iterations, err
}

// IsFitted reports whether Fit has completed successfully.
func (model *KMeans) IsFitted() bool {
	return model != nil && model.fitted
}

func (model *KMeans) initialize(features [][]float64, random *rand.Rand) [][]float64 {
	if model.initMethod == KMeansRandom {
		indices := random.Perm(len(features))[:model.clusterCount]
		centroids := make([][]float64, model.clusterCount)
		for cluster, index := range indices {
			centroids[cluster] = append([]float64(nil), features[index]...)
		}
		return centroids
	}

	centroids := make([][]float64, 0, model.clusterCount)
	centroids = append(centroids, append([]float64(nil), features[random.Intn(len(features))]...))
	minimumDistances := make([]float64, len(features))
	for cluster := 1; cluster < model.clusterCount; cluster++ {
		total := 0.0
		for index, row := range features {
			distance := squaredDistance(row, centroids[len(centroids)-1])
			if len(centroids) > 1 && distance > minimumDistances[index] {
				distance = minimumDistances[index]
			}
			minimumDistances[index] = distance
			total += distance
		}
		chosen := 0
		if total > 0 && !math.IsInf(total, 0) {
			threshold := random.Float64() * total
			for index, distance := range minimumDistances {
				threshold -= distance
				if threshold <= 0 {
					chosen = index
					break
				}
			}
		}
		centroids = append(centroids, append([]float64(nil), features[chosen]...))
	}
	return centroids
}

func (model *KMeans) checkFitted() error {
	if model == nil || !model.fitted {
		return errors.New("coolbox: KMeans model must be fitted before accessing results")
	}
	return nil
}

func validateFloatMatrix(features [][]float64) (int, int, error) {
	if len(features) == 0 || len(features[0]) == 0 {
		return 0, 0, errors.New("coolbox: feature matrix must be non-empty")
	}
	columns := len(features[0])
	for rowIndex, row := range features {
		if len(row) != columns {
			return 0, 0, errors.New("coolbox: feature matrix rows must all have the same length")
		}
		for _, value := range row {
			if math.IsNaN(value) || math.IsInf(value, 0) {
				return 0, 0, fmt.Errorf("coolbox: feature matrix contains a non-finite value in row %d", rowIndex)
			}
		}
	}
	return len(features), columns, nil
}

func assignClusters(features, centroids [][]float64) []int {
	labels := make([]int, len(features))
	for rowIndex, row := range features {
		bestDistance := math.Inf(1)
		for cluster, centroid := range centroids {
			if distance := squaredDistance(row, centroid); distance < bestDistance {
				bestDistance = distance
				labels[rowIndex] = cluster
			}
		}
	}
	return labels
}

func updateCentroids(features [][]float64, labels []int, clusterCount, columns int) [][]float64 {
	centroids := make([][]float64, clusterCount)
	counts := make([]int, clusterCount)
	for cluster := range centroids {
		centroids[cluster] = make([]float64, columns)
	}
	for rowIndex, row := range features {
		cluster := labels[rowIndex]
		counts[cluster]++
		for column, value := range row {
			centroids[cluster][column] += value
		}
	}
	for cluster, count := range counts {
		if count == 0 {
			continue
		}
		for column := range centroids[cluster] {
			centroids[cluster][column] /= float64(count)
		}
	}
	return centroids
}

func clusterInertia(features, centroids [][]float64, labels []int) float64 {
	inertia := 0.0
	for index, row := range features {
		inertia += squaredDistance(row, centroids[labels[index]])
	}
	return inertia
}

func squaredDistance(left, right []float64) float64 {
	distance := 0.0
	for index, value := range left {
		difference := value - right[index]
		distance += difference * difference
	}
	return distance
}

func centroidsClose(left, right [][]float64) bool {
	for row := range left {
		for column, value := range left[row] {
			if math.Abs(value-right[row][column]) > 1e-9 {
				return false
			}
		}
	}
	return true
}

func cloneMatrix(matrix [][]float64) [][]float64 {
	copyOfMatrix := make([][]float64, len(matrix))
	for index, row := range matrix {
		copyOfMatrix[index] = append([]float64(nil), row...)
	}
	return copyOfMatrix
}
