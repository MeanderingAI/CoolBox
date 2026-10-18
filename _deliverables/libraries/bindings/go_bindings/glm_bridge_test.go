package coolbox

import (
	"math"
	"testing"
)

func TestLinearRegressionClosedForm(t *testing.T) {
	method, err := NewLinearRegressionFitMethod(100, 0.01, ClosedForm)
	if err != nil {
		t.Fatal(err)
	}
	model, err := NewLinearRegression(method)
	if err != nil {
		t.Fatal(err)
	}
	defer model.Close()

	if err := model.Fit([][]float64{{1}, {2}, {3}, {4}}, []float64{5, 8, 11, 14}); err != nil {
		t.Fatal(err)
	}
	prediction, err := model.Predict([]float64{5})
	if err != nil {
		t.Fatal(err)
	}
	if math.Abs(prediction-17) > 1e-6 {
		t.Fatalf("Predict([5]) = %v, want 17", prediction)
	}

	weights, intercept, err := model.Coefficients()
	if err != nil {
		t.Fatal(err)
	}
	if len(weights) != 1 || math.Abs(weights[0]-3) > 1e-6 || math.Abs(intercept-2) > 1e-6 {
		t.Fatalf("Coefficients() = (%v, %v), want ([3], 2)", weights, intercept)
	}
}

func TestLinearRegressionRejectsRaggedInput(t *testing.T) {
	_, _, _, err := flattenMatrix([][]float64{{1, 2}, {3}})
	if err == nil {
		t.Fatal("flattenMatrix accepted rows with different lengths")
	}
}

func TestDecisionTreeClassifierAndRegressor(t *testing.T) {
	classifier, err := NewDecisionTree(Gini)
	if err != nil {
		t.Fatal(err)
	}
	defer classifier.Close()
	if err := classifier.Fit([][]int{{0}, {1}, {2}, {3}}, []int{0, 0, 1, 1}, 2); err != nil {
		t.Fatal(err)
	}
	label, err := classifier.Predict([]int{3})
	if err != nil {
		t.Fatal(err)
	}
	if label != 1 {
		t.Fatalf("Predict([3]) = %d, want 1", label)
	}

	regressor, err := NewDecisionTreeRegressor()
	if err != nil {
		t.Fatal(err)
	}
	defer regressor.Close()
	if err := regressor.Fit([][]float64{{0}, {1}, {2}, {3}}, []float64{0.5, 0.5, 4.5, 4.5}, 2); err != nil {
		t.Fatal(err)
	}
	value, err := regressor.Predict([]float64{2.5})
	if err != nil {
		t.Fatal(err)
	}
	if math.Abs(value-4.5) > 1e-9 {
		t.Fatalf("Predict([2.5]) = %v, want 4.5", value)
	}
}

func TestNativeTextProcessor(t *testing.T) {
	lower, err := TextLowercase("CoolBox NLP")
	if err != nil {
		t.Fatal(err)
	}
	if lower != "coolbox nlp" {
		t.Fatalf("TextLowercase() = %q, want %q", lower, "coolbox nlp")
	}
	processed, err := TextProcess("The QUICK foxes, running!")
	if err != nil {
		t.Fatal(err)
	}
	if len(processed) == 0 || processed[0] != "quick" {
		t.Fatalf("TextProcess() = %v, want first token %q", processed, "quick")
	}
}

func TestNativeTimeSeries(t *testing.T) {
	series, err := NewTimeSeries([]float64{1, 2, 3, 4})
	if err != nil {
		t.Fatal(err)
	}
	defer series.Close()
	mean, err := series.Mean()
	if err != nil {
		t.Fatal(err)
	}
	if mean != 2.5 {
		t.Fatalf("Mean() = %v, want 2.5", mean)
	}
	differenced, err := series.Diff(1)
	if err != nil {
		t.Fatal(err)
	}
	defer differenced.Close()
	values, err := differenced.Values()
	if err != nil {
		t.Fatal(err)
	}
	if len(values) != 3 || values[0] != 1 || values[2] != 1 {
		t.Fatalf("Diff(1) = %v, want [1 1 1]", values)
	}
	windows, err := series.CreateWindows(2, 1)
	if err != nil {
		t.Fatal(err)
	}
	if len(windows) != 3 || len(windows[0]) != 2 {
		t.Fatalf("CreateWindows(2, 1) = %v, want 3 windows of 2 values", windows)
	}
}

func TestNativeBanditArm(t *testing.T) {
	arm, err := NewBanditArm(1)
	if err != nil {
		t.Fatal(err)
	}
	defer arm.Close()
	reward, err := arm.Pull()
	if err != nil {
		t.Fatal(err)
	}
	if reward != 1 {
		t.Fatalf("Pull() = %v, want 1 for a probability-one arm", reward)
	}
	if err := arm.Update(1); err != nil {
		t.Fatal(err)
	}
	if err := arm.Update(0); err != nil {
		t.Fatal(err)
	}
	estimate, err := arm.EstimatedProbability()
	if err != nil {
		t.Fatal(err)
	}
	if estimate != 0.5 {
		t.Fatalf("EstimatedProbability() = %v, want 0.5", estimate)
	}
}

func TestNativeForecasters(t *testing.T) {
	movingAverage, err := NewMovingAverageForecaster(2)
	if err != nil {
		t.Fatal(err)
	}
	defer movingAverage.Close()
	if err := movingAverage.Fit([]float64{1, 2, 3}); err != nil {
		t.Fatal(err)
	}
	first, err := movingAverage.ForecastOneStep()
	if err != nil {
		t.Fatal(err)
	}
	if first != 2.5 {
		t.Fatalf("ForecastOneStep() = %v, want 2.5", first)
	}

	arma, err := NewARMAModel(1, 0)
	if err != nil {
		t.Fatal(err)
	}
	defer arma.Close()
	if err := arma.Fit([]float64{1, 2, 3, 4}); err != nil {
		t.Fatal(err)
	}
	predictions, err := arma.Forecast(2)
	if err != nil {
		t.Fatal(err)
	}
	if len(predictions) != 2 {
		t.Fatalf("Forecast(2) returned %d predictions, want 2", len(predictions))
	}
	arima, err := NewARIMAModel(1, 5, 0)
	if err != nil {
		t.Fatal(err)
	}
	defer arima.Close()
	if err := arima.Fit([]float64{1, 2, 3, 4}); err == nil {
		t.Fatal("ARIMA accepted a differencing order not smaller than the sample count")
	}
}

func TestKMeans(t *testing.T) {
	model, err := NewKMeans(2, 50, KMeansPlusPlus, 42)
	if err != nil {
		t.Fatal(err)
	}
	features := [][]float64{{0}, {1}, {9}, {10}}
	labels, err := model.FitPredict(features)
	if err != nil {
		t.Fatal(err)
	}
	if len(labels) != len(features) || labels[0] != labels[1] || labels[2] != labels[3] || labels[0] == labels[2] {
		t.Fatalf("FitPredict() labels = %v, want the two separated pairs clustered separately", labels)
	}
	predictions, err := model.Predict([][]float64{{0.5}, {9.5}})
	if err != nil {
		t.Fatal(err)
	}
	if predictions[0] != labels[0] || predictions[1] != labels[2] {
		t.Fatalf("Predict() = %v, inconsistent with fitted labels %v", predictions, labels)
	}
	result, err := model.Result()
	if err != nil {
		t.Fatal(err)
	}
	result.Centroids[0][0] = 999
	centroids, err := model.Centroids()
	if err != nil {
		t.Fatal(err)
	}
	if centroids[0][0] == 999 {
		t.Fatal("Centroids() exposed mutable internal state")
	}
}

func TestKNNMetricsAndNeighbors(t *testing.T) {
	model, err := NewKNN(1, "manhattan")
	if err != nil {
		t.Fatal(err)
	}
	if err := model.Fit([][]float64{{0, 0}, {2, 0}, {0, 3}}); err != nil {
		t.Fatal(err)
	}
	indices, distances, err := model.KNeighbors([][]float64{{1.5, 0}})
	if err != nil {
		t.Fatal(err)
	}
	if indices[0][0] != 1 || distances[0][0] != 0.5 {
		t.Fatalf("KNeighbors() = (%v, %v), want nearest row 1 at distance 0.5", indices, distances)
	}
	trainIndices, _, err := model.KNeighborsTraining()
	if err != nil {
		t.Fatal(err)
	}
	if trainIndices[0][0] != 1 {
		t.Fatalf("KNeighborsTraining()[0] = %v, want nearest row 1", trainIndices[0])
	}
}

func TestClientAndUUIDBindings(t *testing.T) {
	client, err := NewClientForEndpoint("")
	if err != nil {
		t.Fatal(err)
	}
	defer client.Close()

	endpoint, err := client.Endpoint()
	if err != nil {
		t.Fatal(err)
	}
	if endpoint != DefaultEndpoint() {
		t.Fatalf("Endpoint() = %q, want %q", endpoint, DefaultEndpoint())
	}
	if UUIDV4() == "" {
		t.Fatal("UUIDV4 returned an empty string")
	}
}
