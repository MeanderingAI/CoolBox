package coolbox

import (
	"math"
	"os"
	"path/filepath"
	"testing"
)

func TestTensorOperations(t *testing.T) {
	tensor, err := NewTensorFromData([]int{2, 2}, []float64{1, 2, 3, 4})
	if err != nil {
		t.Fatal(err)
	}
	defer tensor.Close()

	transposed, err := tensor.Transpose()
	if err != nil {
		t.Fatal(err)
	}
	defer transposed.Close()
	values, err := transposed.Values()
	if err != nil {
		t.Fatal(err)
	}
	if len(values) != 4 || values[0] != 1 || values[1] != 3 || values[2] != 2 || values[3] != 4 {
		t.Fatalf("Transpose() = %v, want [1 3 2 4]", values)
	}

	product, err := tensor.MatMul(transposed)
	if err != nil {
		t.Fatal(err)
	}
	defer product.Close()
	productValues, err := product.Values()
	if err != nil {
		t.Fatal(err)
	}
	want := []float64{5, 11, 11, 25}
	for i := range want {
		if productValues[i] != want[i] {
			t.Fatalf("MatMul()[%d] = %v, want %v", i, productValues[i], want[i])
		}
	}

	if err := tensor.Set(0, 9); err != nil {
		t.Fatal(err)
	}
	first, err := tensor.At(0)
	if err != nil || first != 9 {
		t.Fatalf("At(0) = %v, %v; want 9, nil", first, err)
	}
	denominator, err := NewTensorFromData([]int{2, 2}, []float64{1, 0, 1, 1})
	if err != nil {
		t.Fatal(err)
	}
	defer denominator.Close()
	if _, err := tensor.Div(denominator); err == nil {
		t.Fatal("Div() accepted a zero denominator")
	}
}

func TestDeepLearningLossOptimizerAndNetwork(t *testing.T) {
	predictions, err := NewTensorFromData([]int{1, 2}, []float64{2, 4})
	if err != nil {
		t.Fatal(err)
	}
	defer predictions.Close()
	targets, err := NewTensorFromData([]int{1, 2}, []float64{1, 2})
	if err != nil {
		t.Fatal(err)
	}
	defer targets.Close()

	loss, err := NewLoss(MeanSquaredError)
	if err != nil {
		t.Fatal(err)
	}
	defer loss.Close()
	value, err := loss.Compute(predictions, targets)
	if err != nil {
		t.Fatal(err)
	}
	if value != 2.5 {
		t.Fatalf("MSE = %v, want 2.5", value)
	}
	gradient, err := loss.Gradient(predictions, targets)
	if err != nil {
		t.Fatal(err)
	}
	defer gradient.Close()
	gradientValues, err := gradient.Values()
	if err != nil {
		t.Fatal(err)
	}
	if gradientValues[0] != 1 || gradientValues[1] != 2 {
		t.Fatalf("MSE gradient = %v, want [1 2]", gradientValues)
	}

	optimizer, err := NewSGD(0.1, 0)
	if err != nil {
		t.Fatal(err)
	}
	defer optimizer.Close()
	parameters, err := NewTensorFromData([]int{2}, []float64{3, -1})
	if err != nil {
		t.Fatal(err)
	}
	defer parameters.Close()
	parameterGradient, err := NewTensorFromData([]int{2}, []float64{2, -4})
	if err != nil {
		t.Fatal(err)
	}
	defer parameterGradient.Close()
	if err := optimizer.Step(parameters, parameterGradient); err != nil {
		t.Fatal(err)
	}
	updated, err := parameters.Values()
	if err != nil {
		t.Fatal(err)
	}
	if updated[0] != 2.8 || updated[1] != -0.6 {
		t.Fatalf("SGD parameters = %v, want [2.8 -0.6]", updated)
	}

	network, err := NewNeuralNetwork()
	if err != nil {
		t.Fatal(err)
	}
	defer network.Close()
	dense, err := NewDenseLayer(2, 1)
	if err != nil {
		t.Fatal(err)
	}
	defer dense.Close()
	if err := network.AddLayer(dense); err != nil {
		t.Fatal(err)
	}
	if err := network.SetLoss(loss); err != nil {
		t.Fatal(err)
	}
	input, err := NewTensorFromData([]int{1, 2}, []float64{1, 0})
	if err != nil {
		t.Fatal(err)
	}
	defer input.Close()
	prediction, err := network.Predict(input)
	if err != nil {
		t.Fatal(err)
	}
	defer prediction.Close()
	predictionShape, err := prediction.Shape()
	if err != nil {
		t.Fatal(err)
	}
	if len(predictionShape) != 2 || predictionShape[0] != 1 || predictionShape[1] != 1 {
		t.Fatalf("network output shape = %v, want [1 1]", predictionShape)
	}
	layerCount, err := network.LayerCount()
	if err != nil || layerCount != 1 {
		t.Fatalf("LayerCount() = %d, %v; want 1, nil", layerCount, err)
	}
	networkTarget, err := NewTensorFromData([]int{1, 1}, []float64{0.5})
	if err != nil {
		t.Fatal(err)
	}
	defer networkTarget.Close()
	if err := network.Train([]*Tensor{input}, []*Tensor{networkTarget}, 1, 1, false); err != nil {
		t.Fatal(err)
	}
	evaluation, err := network.Evaluate([]*Tensor{input}, []*Tensor{networkTarget})
	if err != nil {
		t.Fatal(err)
	}
	if math.IsNaN(evaluation) || math.IsInf(evaluation, 0) {
		t.Fatalf("Evaluate() returned non-finite loss %v", evaluation)
	}
}

func TestDeepLearningLayerFactories(t *testing.T) {
	factories := []struct {
		name string
		make func() (*Layer, error)
	}{
		{"relu", NewReLULayer},
		{"sigmoid", NewSigmoidLayer},
		{"tanh", NewTanhLayer},
		{"softmax", NewSoftmaxLayer},
		{"dropout", func() (*Layer, error) { return NewDropoutLayer(0.25) }},
		{"batchnorm", func() (*Layer, error) { return NewBatchNormLayer(3) }},
		{"layernorm", func() (*Layer, error) { return NewLayerNormLayer(3) }},
		{"flatten", NewFlattenLayer},
		{"conv1d", func() (*Layer, error) { return NewConv1DLayer(1, 2, 3) }},
		{"conv2d", func() (*Layer, error) { return NewConv2DLayer(1, 2, 3, 3, 1, 1) }},
		{"maxpool2d", func() (*Layer, error) { return NewMaxPool2DLayer(2, 2, 2, 2) }},
		{"avgpool2d", func() (*Layer, error) { return NewAvgPool2DLayer(2, 2, 2, 2) }},
		{"rnn", func() (*Layer, error) { return NewRNNLayer(2, 3, true) }},
		{"lstm", func() (*Layer, error) { return NewLSTMLayer(2, 3, true) }},
		{"gru", func() (*Layer, error) { return NewGRULayer(2, 3, true) }},
		{"embedding", func() (*Layer, error) { return NewEmbeddingLayer(8, 4) }},
	}
	for _, factory := range factories {
		t.Run(factory.name, func(t *testing.T) {
			layer, err := factory.make()
			if err != nil {
				t.Fatal(err)
			}
			defer layer.Close()
			name, err := layer.Name()
			if err != nil || name == "" {
				t.Fatalf("Name() = %q, %v", name, err)
			}
		})
	}

	dropout, err := NewDropoutLayer(0.5)
	if err != nil {
		t.Fatal(err)
	}
	defer dropout.Close()
	if err := dropout.SetTraining(false); err != nil {
		t.Fatal(err)
	}
	input, err := NewTensorFromData([]int{1, 2}, []float64{0.25, 0.75})
	if err != nil {
		t.Fatal(err)
	}
	defer input.Close()
	output, err := dropout.Forward(input)
	if err != nil {
		t.Fatal(err)
	}
	defer output.Close()
	values, err := output.Values()
	if err != nil {
		t.Fatal(err)
	}
	if values[0] != 0.25 || values[1] != 0.75 {
		t.Fatalf("dropout inference output = %v, want unchanged input", values)
	}
}

func TestHelixAgentBindings(t *testing.T) {
	dimension, err := TransferEluderDimension(4, 2, RichLanguageFeedback, 1e-9, 18)
	if err != nil {
		t.Fatal(err)
	}
	if dimension != 1 {
		t.Fatalf("rich-feedback transfer eluder dimension = %d, want 1", dimension)
	}

	learner, err := NewHelix(4, 2, RichLanguageFeedback)
	if err != nil {
		t.Fatal(err)
	}
	defer learner.Close()
	action, err := learner.Interact()
	if err != nil {
		t.Fatal(err)
	}
	if action < 0 || action >= 4 {
		t.Fatalf("Interact() = %d, outside [0, 4)", action)
	}
	rounds, err := learner.Rounds()
	if err != nil || rounds != 1 {
		t.Fatalf("Rounds() = %d, %v; want 1, nil", rounds, err)
	}
	regret, err := learner.CumulativeRegret()
	if err != nil || math.IsNaN(regret) || regret < 0 {
		t.Fatalf("CumulativeRegret() = %v, %v", regret, err)
	}
}

func TestGraphicsLineChartExport(t *testing.T) {
	graph, err := NewGraph(320, 240, LineGraph)
	if err != nil {
		t.Fatal(err)
	}
	defer graph.Close()
	if err := graph.SetTitle("Training Loss"); err != nil {
		t.Fatal(err)
	}
	if err := graph.SetXLabel("Epoch"); err != nil {
		t.Fatal(err)
	}
	if err := graph.SetYLabel("Loss"); err != nil {
		t.Fatal(err)
	}
	if err := graph.AddSeries(Series{
		Label: "train",
		X:     []float64{1, 2, 3, 4},
		Y:     []float64{0.9, 0.6, 0.3, 0.1},
		Color: Blue,
	}); err != nil {
		t.Fatal(err)
	}
	canvas, err := graph.Render()
	if err != nil {
		t.Fatal(err)
	}
	defer canvas.Close()
	width, err := canvas.Width()
	if err != nil {
		t.Fatal(err)
	}
	height, err := canvas.Height()
	if err != nil {
		t.Fatal(err)
	}
	if width != 320 || height != 240 {
		t.Fatalf("rendered canvas is %dx%d, want 320x240", width, height)
	}
	pixels, err := canvas.RGBA()
	if err != nil {
		t.Fatal(err)
	}
	if len(pixels) != width*height*4 {
		t.Fatalf("RGBA() returned %d bytes, want %d", len(pixels), width*height*4)
	}

	path := filepath.Join(t.TempDir(), "loss.png")
	if err := canvas.SavePNG(path); err != nil {
		t.Fatal(err)
	}
	image, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	if len(image) < 8 || string(image[:8]) != "\x89PNG\r\n\x1a\n" {
		t.Fatal("SavePNG() did not produce a PNG image")
	}
}

func TestGraphicsBarAndScatterPlots(t *testing.T) {
	for _, kind := range []GraphType{BarGraph, ScatterGraph} {
		graph, err := NewGraph(240, 180, kind)
		if err != nil {
			t.Fatal(err)
		}
		if err := graph.AddSeries(Series{
			X: []float64{2, 2, 2},
			Y: []float64{1, 3, 2},
		}); err != nil {
			graph.Close()
			t.Fatal(err)
		}
		canvas, err := graph.Render()
		graph.Close()
		if err != nil {
			t.Fatalf("render graph type %d: %v", kind, err)
		}
		if err := canvas.Close(); err != nil {
			t.Fatal(err)
		}
	}
}
