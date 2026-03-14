package coolboxgo

import "testing"

func TestLinearRegressionClosedForm(t *testing.T) {
	x := [][]float64{{1}, {2}, {3}, {4}}
	y := []float64{3, 5, 7, 9}

	model, err := FitLinearRegression(x, y, FitMethodClosedForm, 500, 0.01)
	if err != nil {
		t.Fatalf("fit failed: %v", err)
	}
	defer model.Close()

	preds, err := model.Predict(x)
	if err != nil {
		t.Fatalf("predict failed: %v", err)
	}
	if len(preds) != len(y) {
		t.Fatalf("unexpected prediction length: %d", len(preds))
	}
}

func TestDecisionTreeSmoke(t *testing.T) {
	tree, err := NewDecisionTree(SplitCriterionGini)
	if err != nil {
		t.Fatalf("new decision tree failed: %v", err)
	}
	defer tree.Close()

	x := [][]int{{0, 0}, {0, 1}, {1, 0}, {1, 1}}
	y := []int{0, 0, 1, 1}

	if err := tree.Fit(x, y, 3); err != nil {
		t.Fatalf("fit failed: %v", err)
	}

	label, err := tree.Predict([]int{1, 0})
	if err != nil {
		t.Fatalf("predict failed: %v", err)
	}
	if label != 1 {
		t.Fatalf("unexpected label: %d", label)
	}
}
