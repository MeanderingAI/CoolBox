package coolboxgo

import "fmt"

func bindingStubError(name string) error {
	return fmt.Errorf("%s is not implemented in the Go bindings yet", name)
}

func (h *HMM) GetMostLikelyStates(observations []int) ([]int, error) {
	return h.MostLikelyStates(observations)
}

func (b *BanditArm) GetEstimatedProb() float64 {
	return b.EstimatedProbability()
}

func (b *BanditArm) GetPullCount() int {
	return b.PullCount()
}

func (b *BanditArm) GetTrueProb() float64 {
	return b.TrueProbability()
}

func (a *BanditAgent) GetResults() (SimulationResult, error) {
	return a.Results()
}

type RandomForest struct {
	numTrees int
	maxDepth int
}

func NewRandomForest(numTrees, maxDepth int) *RandomForest {
	return &RandomForest{numTrees: numTrees, maxDepth: maxDepth}
}

func (r *RandomForest) Fit(x [][]int, y []int) error {
	return bindingStubError("RandomForest.Fit")
}

func (r *RandomForest) Predict(sample []int) (int, error) {
	return 0, bindingStubError("RandomForest.Predict")
}

type BoostTreeParameters struct {
	NumEstimators uint32
	LearningRate  float64
	MaxDepth      uint32
}

type BoostTree struct {
	params BoostTreeParameters
}

func NewBoostTree(params BoostTreeParameters) *BoostTree {
	if params.NumEstimators == 0 {
		params.NumEstimators = 100
	}
	if params.LearningRate == 0 {
		params.LearningRate = 0.1
	}
	if params.MaxDepth == 0 {
		params.MaxDepth = 3
	}
	return &BoostTree{params: params}
}

func (b *BoostTree) Fit(x [][]float64, y []float64) error {
	return bindingStubError("BoostTree.Fit")
}

func (b *BoostTree) Predict(sample []float64) (float64, error) {
	return 0, bindingStubError("BoostTree.Predict")
}

type MarkedPointProcess struct {
	numMarks      int
	learningRate  float64
	maxIterations int
}

func NewMarkedPointProcess(numMarks int, learningRate float64, maxIterations int) *MarkedPointProcess {
	return &MarkedPointProcess{
		numMarks:      numMarks,
		learningRate:  learningRate,
		maxIterations: maxIterations,
	}
}

func (m *MarkedPointProcess) Fit(eventTimes [][]float64, eventMarks [][]int) error {
	return bindingStubError("MarkedPointProcess.Fit")
}

type LatentSentimentAnalysis struct {
	numFeatures int
}

func NewLatentSentimentAnalysis(numFeatures int) *LatentSentimentAnalysis {
	return &LatentSentimentAnalysis{numFeatures: numFeatures}
}

func (l *LatentSentimentAnalysis) Train(dtm [][]float64) error {
	return bindingStubError("LatentSentimentAnalysis.Train")
}

func (l *LatentSentimentAnalysis) PredictScore(docIndex, termIndex int) (float64, error) {
	return 0, bindingStubError("LatentSentimentAnalysis.PredictScore")
}
