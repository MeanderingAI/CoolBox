package coolbox

import (
	"math"
	"testing"
)

func TestBoostTreeRegression(t *testing.T) {
	model, err := NewBoostTree(BoostTreeParameters{
		NumEstimators: 10,
		LearningRate:  0.1,
		MaxDepth:      0,
	})
	if err != nil {
		t.Fatal(err)
	}
	defer model.Close()

	features := [][]float64{{0}, {1}, {2}, {3}}
	if err := model.Fit(features, []float64{1, 2, 3, 4}); err != nil {
		t.Fatal(err)
	}
	predictions, err := model.PredictBatch(features)
	if err != nil {
		t.Fatal(err)
	}
	if len(predictions) != 4 {
		t.Fatalf("PredictBatch() returned %d values, want 4", len(predictions))
	}
	for _, prediction := range predictions {
		if math.Abs(prediction-2.5) > 1e-9 {
			t.Fatalf("constant-model prediction = %v, want 2.5", prediction)
		}
	}
	prediction, err := model.Predict([]float64{1.5})
	if err != nil {
		t.Fatal(err)
	}
	if math.Abs(prediction-2.5) > 1e-9 {
		t.Fatalf("Predict() = %v, want 2.5", prediction)
	}
}

func TestBoostTreeClassifierArbitraryLabels(t *testing.T) {
	model, err := NewBoostTreeClassifier(BoostTreeParameters{
		NumEstimators: 20,
		LearningRate:  0.2,
		MaxDepth:      1,
	})
	if err != nil {
		t.Fatal(err)
	}
	defer model.Close()

	if err := model.Fit(
		[][]float64{{0}, {0.1}, {1}, {1.1}},
		[]int{-7, -7, 42, 42},
	); err != nil {
		t.Fatal(err)
	}
	classes, err := model.Classes()
	if err != nil {
		t.Fatal(err)
	}
	if len(classes) != 2 || classes[0] != -7 || classes[1] != 42 {
		t.Fatalf("Classes() = %v, want [-7 42]", classes)
	}
	for _, sample := range []struct {
		features []float64
		want     int
	}{
		{[]float64{0}, -7},
		{[]float64{1.1}, 42},
	} {
		prediction, err := model.Predict(sample.features)
		if err != nil {
			t.Fatal(err)
		}
		if prediction != sample.want {
			t.Fatalf("Predict(%v) = %d, want %d", sample.features, prediction, sample.want)
		}
		probabilities, err := model.PredictProba(sample.features)
		if err != nil {
			t.Fatal(err)
		}
		if len(probabilities) != len(classes) {
			t.Fatalf("PredictProba() returned %d values for %d classes", len(probabilities), len(classes))
		}
		total := 0.0
		for _, probability := range probabilities {
			if probability < 0 || probability > 1 {
				t.Fatalf("invalid class probability %v", probability)
			}
			total += probability
		}
		if math.Abs(total-1) > 1e-12 {
			t.Fatalf("class probabilities sum to %v, want 1", total)
		}
	}
}

func TestBoostTreeRejectsInvalidInput(t *testing.T) {
	if _, err := NewBoostTree(BoostTreeParameters{}); err == nil {
		t.Fatal("NewBoostTree accepted zero-valued parameters")
	}
	model, err := NewBoostTree(DefaultBoostTreeParameters())
	if err != nil {
		t.Fatal(err)
	}
	defer model.Close()
	if err := model.Fit([][]float64{{1}, {2}}, []float64{1}); err == nil {
		t.Fatal("Fit accepted a target count that does not match the feature rows")
	}
	if _, err := model.Predict([]float64{1}); err == nil {
		t.Fatal("Predict succeeded before fitting")
	}
}
