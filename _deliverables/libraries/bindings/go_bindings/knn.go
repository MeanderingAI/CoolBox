package coolbox

import (
	"errors"
	"fmt"
	"math"
	"sort"
)

// KNN stores training rows for exact brute-force nearest-neighbor queries.
type KNN struct {
	k        int
	metric   string
	training [][]float64
	fitted   bool
}

// NewKNN creates a nearest-neighbor index using euclidean, manhattan, or cosine distance.
func NewKNN(k int, metric string) (*KNN, error) {
	if k <= 0 {
		return nil, errors.New("coolbox: k must be positive")
	}
	switch metric {
	case "euclidean", "manhattan", "cosine":
	default:
		return nil, fmt.Errorf("coolbox: unsupported KNN metric %q", metric)
	}
	return &KNN{k: k, metric: metric}, nil
}

// Fit stores a copy of the training data.
func (model *KNN) Fit(features [][]float64) error {
	if model == nil {
		return errors.New("coolbox: KNN model is nil")
	}
	if len(features) <= model.k {
		return errors.New("coolbox: k must be less than the number of training samples")
	}
	if _, _, err := validateFloatMatrix(features); err != nil {
		return err
	}
	model.training = cloneMatrix(features)
	model.fitted = true
	return nil
}

// KNeighbors returns the closest training rows for each query.
func (model *KNN) KNeighbors(queries [][]float64) ([][]int, [][]float64, error) {
	if err := model.checkFitted(); err != nil {
		return nil, nil, err
	}
	if len(queries) == 0 {
		return [][]int{}, [][]float64{}, nil
	}
	_, columns, err := validateFloatMatrix(queries)
	if err != nil {
		return nil, nil, err
	}
	if columns != len(model.training[0]) {
		return nil, nil, errors.New("coolbox: query feature count does not match training data")
	}
	return model.neighbors(queries, nil)
}

// KNeighborsTraining returns each training row's neighbors excluding itself.
func (model *KNN) KNeighborsTraining() ([][]int, [][]float64, error) {
	if err := model.checkFitted(); err != nil {
		return nil, nil, err
	}
	self := make([]int, len(model.training))
	for index := range self {
		self[index] = index
	}
	return model.neighbors(model.training, self)
}

// PairwiseDistances computes distances from rows in x to rows in y.
func (model *KNN) PairwiseDistances(x, y [][]float64) ([][]float64, error) {
	if model == nil {
		return nil, errors.New("coolbox: KNN model is nil")
	}
	_, xColumns, err := validateFloatMatrix(x)
	if err != nil {
		return nil, err
	}
	_, yColumns, err := validateFloatMatrix(y)
	if err != nil {
		return nil, err
	}
	if xColumns != yColumns {
		return nil, errors.New("coolbox: both matrices must have the same feature count")
	}
	distances := make([][]float64, len(x))
	for rowIndex, row := range x {
		distances[rowIndex] = make([]float64, len(y))
		for columnIndex, other := range y {
			distances[rowIndex][columnIndex] = model.distance(row, other)
		}
	}
	return distances, nil
}

// K returns the number of neighbors requested.
func (model *KNN) K() int { return model.k }

// Metric returns the configured distance metric.
func (model *KNN) Metric() string { return model.metric }

// IsFitted reports whether the training data has been set.
func (model *KNN) IsFitted() bool { return model != nil && model.fitted }

type neighbor struct {
	index    int
	distance float64
}

func (model *KNN) neighbors(queries [][]float64, excluded []int) ([][]int, [][]float64, error) {
	available := len(model.training)
	if excluded != nil {
		available--
	}
	if model.k > available {
		return nil, nil, errors.New("coolbox: fewer than k training samples are available")
	}
	indices := make([][]int, len(queries))
	distances := make([][]float64, len(queries))
	for queryIndex, query := range queries {
		candidates := make([]neighbor, 0, len(model.training))
		for trainIndex, train := range model.training {
			if excluded != nil && trainIndex == excluded[queryIndex] {
				continue
			}
			candidates = append(candidates, neighbor{
				index:    trainIndex,
				distance: model.distance(query, train),
			})
		}
		sort.Slice(candidates, func(i, j int) bool {
			if candidates[i].distance == candidates[j].distance {
				return candidates[i].index < candidates[j].index
			}
			return candidates[i].distance < candidates[j].distance
		})
		indices[queryIndex] = make([]int, model.k)
		distances[queryIndex] = make([]float64, model.k)
		for index := 0; index < model.k; index++ {
			indices[queryIndex][index] = candidates[index].index
			distances[queryIndex][index] = candidates[index].distance
		}
	}
	return indices, distances, nil
}

func (model *KNN) distance(left, right []float64) float64 {
	switch model.metric {
	case "manhattan":
		distance := 0.0
		for index, value := range left {
			distance += math.Abs(value - right[index])
		}
		return distance
	case "cosine":
		dot, leftNorm, rightNorm := 0.0, 0.0, 0.0
		for index, value := range left {
			dot += value * right[index]
			leftNorm += value * value
			rightNorm += right[index] * right[index]
		}
		if leftNorm == 0 || rightNorm == 0 {
			return 1
		}
		similarity := dot / (math.Sqrt(leftNorm) * math.Sqrt(rightNorm))
		similarity = math.Max(-1, math.Min(1, similarity))
		return 1 - similarity
	default:
		return math.Sqrt(squaredDistance(left, right))
	}
}

func (model *KNN) checkFitted() error {
	if model == nil || !model.fitted {
		return errors.New("coolbox: KNN must be fitted before querying neighbors")
	}
	return nil
}
