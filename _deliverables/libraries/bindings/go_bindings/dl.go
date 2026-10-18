package coolbox

/*
#include "dl_bridge.h"
#include <stdlib.h>
*/
import "C"

import (
	"errors"
	"fmt"
	"math"
	"strings"
	"unsafe"
)

func dlNativeError(buffer *[nativeErrorCapacity]C.char) error {
	message := C.GoString(&buffer[0])
	if message == "" {
		message = "native operation failed"
	}
	return errors.New(message)
}

func checkedShape(shape []int) ([]C.size_t, error) {
	if len(shape) == 0 {
		return nil, errors.New("tensor shape must have at least one dimension")
	}
	dimensions := make([]C.size_t, len(shape))
	total := 1
	for i, dimension := range shape {
		if dimension <= 0 || total > math.MaxInt/dimension {
			return nil, errors.New("tensor dimensions must be positive and not overflow")
		}
		total *= dimension
		dimensions[i] = C.size_t(dimension)
	}
	return dimensions, nil
}

func sizePointer(values []C.size_t) *C.size_t {
	if len(values) == 0 {
		return nil
	}
	return &values[0]
}

func doublePointer(values []float64) *C.double {
	if len(values) == 0 {
		return nil
	}
	return (*C.double)(unsafe.Pointer(&values[0]))
}

func cLayerParameters(values []int) ([]C.size_t, error) {
	parameters := make([]C.size_t, len(values))
	for i, value := range values {
		if value < 0 {
			return nil, fmt.Errorf("layer parameter %d must not be negative", i)
		}
		parameters[i] = C.size_t(value)
	}
	return parameters, nil
}

// Tensor is a multidimensional array backed by the native deep-learning library.
// Close releases its native allocation; methods on a closed tensor return errors.
type Tensor struct {
	handle unsafe.Pointer
}

// NewTensor allocates a zero-filled tensor with the supplied positive dimensions.
func NewTensor(shape []int) (*Tensor, error) {
	return newTensor(shape, nil)
}

// NewTensorFromData creates a tensor and copies values in row-major order.
func NewTensorFromData(shape []int, values []float64) (*Tensor, error) {
	_, err := checkedShape(shape)
	if err != nil {
		return nil, err
	}
	size := 1
	for _, dimension := range shape {
		if size > math.MaxInt/dimension {
			return nil, errors.New("tensor dimensions overflow")
		}
		size *= dimension
	}
	if len(values) != size {
		return nil, fmt.Errorf("tensor has %d values, want %d", len(values), size)
	}
	return newTensor(shape, values)
}

func newTensor(shape []int, values []float64) (*Tensor, error) {
	dimensions, err := checkedShape(shape)
	if err != nil {
		return nil, err
	}
	if len(values) != 0 {
		size := 1
		for _, dimension := range shape {
			if size > math.MaxInt/dimension {
				return nil, errors.New("tensor dimensions overflow")
			}
			size *= dimension
		}
		if len(values) != size {
			return nil, fmt.Errorf("tensor has %d values, want %d", len(values), size)
		}
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_tensor_new(
		sizePointer(dimensions),
		C.size_t(len(dimensions)),
		doublePointer(values),
		C.size_t(len(values)),
		&nativeErr[0],
		C.size_t(len(nativeErr)),
	)
	if handle == nil {
		return nil, dlNativeError(&nativeErr)
	}
	return &Tensor{handle: handle}, nil
}

func (t *Tensor) valid() error {
	if t == nil || t.handle == nil {
		return errors.New("tensor is closed")
	}
	return nil
}

// Close releases the native tensor. It is safe to call Close more than once.
func (t *Tensor) Close() error {
	if t != nil && t.handle != nil {
		C.coolbox_dl_tensor_free(t.handle)
		t.handle = nil
	}
	return nil
}

// Shape returns the dimensions of the tensor.
func (t *Tensor) Shape() ([]int, error) {
	if err := t.valid(); err != nil {
		return nil, err
	}
	count := int(C.coolbox_dl_tensor_dimensions(t.handle))
	shape := make([]C.size_t, count)
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_tensor_shape(t.handle, sizePointer(shape), C.size_t(count),
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return nil, dlNativeError(&nativeErr)
	}
	result := make([]int, count)
	for i, dimension := range shape {
		result[i] = int(dimension)
	}
	return result, nil
}

// Values returns a copy of the row-major tensor data.
func (t *Tensor) Values() ([]float64, error) {
	if err := t.valid(); err != nil {
		return nil, err
	}
	count := int(C.coolbox_dl_tensor_size(t.handle))
	values := make([]float64, count)
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_tensor_data(t.handle, doublePointer(values), C.size_t(count),
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return nil, dlNativeError(&nativeErr)
	}
	return values, nil
}

func tensorFromHandle(handle unsafe.Pointer, nativeErr *[nativeErrorCapacity]C.char) (*Tensor, error) {
	if handle == nil {
		return nil, dlNativeError(nativeErr)
	}
	return &Tensor{handle: handle}, nil
}

// Clone returns an independent copy of the tensor.
func (t *Tensor) Clone() (*Tensor, error) {
	if err := t.valid(); err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_tensor_clone(t.handle, &nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

// Reshape returns a view-copy with the requested shape and unchanged data.
func (t *Tensor) Reshape(shape []int) (*Tensor, error) {
	if err := t.valid(); err != nil {
		return nil, err
	}
	dimensions, err := checkedShape(shape)
	if err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_tensor_reshape(t.handle, sizePointer(dimensions), C.size_t(len(dimensions)),
		&nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

// Transpose transposes a two-dimensional tensor.
func (t *Tensor) Transpose() (*Tensor, error) {
	if err := t.valid(); err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_tensor_transpose(t.handle, &nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

func (t *Tensor) binary(other *Tensor, operation int) (*Tensor, error) {
	if err := t.valid(); err != nil {
		return nil, err
	}
	if err := other.valid(); err != nil {
		return nil, err
	}
	leftShape, err := t.Shape()
	if err != nil {
		return nil, err
	}
	rightShape, err := other.Shape()
	if err != nil {
		return nil, err
	}
	if len(leftShape) != len(rightShape) {
		return nil, errors.New("tensor shapes must match for element-wise operations")
	}
	for i := range leftShape {
		if leftShape[i] != rightShape[i] {
			return nil, errors.New("tensor shapes must match for element-wise operations")
		}
	}
	if operation == 3 {
		values, err := other.Values()
		if err != nil {
			return nil, err
		}
		for i, value := range values {
			if math.Abs(value) < 1e-10 {
				return nil, fmt.Errorf("element-wise division by zero at index %d", i)
			}
		}
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_tensor_binary(t.handle, other.handle, C.int(operation),
		&nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

// Add performs element-wise addition.
func (t *Tensor) Add(other *Tensor) (*Tensor, error) { return t.binary(other, 0) }

// Sub performs element-wise subtraction.
func (t *Tensor) Sub(other *Tensor) (*Tensor, error) { return t.binary(other, 1) }

// Mul performs element-wise multiplication.
func (t *Tensor) Mul(other *Tensor) (*Tensor, error) { return t.binary(other, 2) }

// Div performs element-wise division.
func (t *Tensor) Div(other *Tensor) (*Tensor, error) { return t.binary(other, 3) }

// Scale multiplies every element by a scalar.
func (t *Tensor) Scale(scalar float64) (*Tensor, error) {
	return t.scalar(scalar, false)
}

// DivideScalar divides every element by a non-zero scalar.
func (t *Tensor) DivideScalar(scalar float64) (*Tensor, error) {
	return t.scalar(scalar, true)
}

func (t *Tensor) scalar(value float64, divide bool) (*Tensor, error) {
	if err := t.valid(); err != nil {
		return nil, err
	}
	if divide && math.Abs(value) < 1e-10 {
		return nil, errors.New("division by zero")
	}
	var nativeErr [nativeErrorCapacity]C.char
	divideFlag := C.int(0)
	if divide {
		divideFlag = 1
	}
	handle := C.coolbox_dl_tensor_scalar(t.handle, C.double(value), divideFlag,
		&nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

// MatMul performs two-dimensional matrix multiplication.
func (t *Tensor) MatMul(other *Tensor) (*Tensor, error) {
	if err := t.valid(); err != nil {
		return nil, err
	}
	if err := other.valid(); err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_tensor_matmul(t.handle, other.handle, &nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

// Fill overwrites every element with value.
func (t *Tensor) Fill(value float64) error {
	if err := t.valid(); err != nil {
		return err
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_tensor_fill(t.handle, C.double(value), &nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

// Randomize fills the tensor with uniform values in [minimum, maximum).
func (t *Tensor) Randomize(minimum, maximum float64) error {
	if err := t.valid(); err != nil {
		return err
	}
	if math.IsNaN(minimum) || math.IsInf(minimum, 0) ||
		math.IsNaN(maximum) || math.IsInf(maximum, 0) {
		return errors.New("randomize bounds must be finite")
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_tensor_randomize(t.handle, C.double(minimum), C.double(maximum),
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

// At returns the value at a flattened row-major index.
func (t *Tensor) At(index int) (float64, error) {
	if err := t.valid(); err != nil {
		return 0, err
	}
	if index < 0 {
		return 0, errors.New("tensor index must not be negative")
	}
	var value C.double
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_tensor_at(t.handle, C.size_t(index), &value, &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return 0, dlNativeError(&nativeErr)
	}
	return float64(value), nil
}

// Set updates the value at a flattened row-major index.
func (t *Tensor) Set(index int, value float64) error {
	if err := t.valid(); err != nil {
		return err
	}
	if index < 0 {
		return errors.New("tensor index must not be negative")
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_tensor_set(t.handle, C.size_t(index), C.double(value),
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

type layerKind C.int

const (
	layerDense layerKind = iota
	layerReLU
	layerSigmoid
	layerTanh
	layerSoftmax
	layerDropout
	layerBatchNorm
	layerNorm
	layerFlatten
	layerConv1D
	layerConv2D
	layerMaxPool2D
	layerAvgPool2D
	layerRNN
	layerLSTM
	layerGRU
	layerEmbedding
)

// Layer wraps a native deep-learning layer.
type Layer struct {
	handle unsafe.Pointer
}

func newLayer(kind layerKind, parameters []int, rate float64, returnSequences bool) (*Layer, error) {
	nativeParams, err := cLayerParameters(parameters)
	if err != nil {
		return nil, err
	}
	returnSequencesFlag := C.int(0)
	if returnSequences {
		returnSequencesFlag = 1
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_layer_new(C.int(kind), sizePointer(nativeParams), C.size_t(len(nativeParams)),
		C.double(rate), returnSequencesFlag, &nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, dlNativeError(&nativeErr)
	}
	return &Layer{handle: handle}, nil
}

func NewDenseLayer(inputSize, outputSize int) (*Layer, error) {
	if inputSize <= 0 || outputSize <= 0 {
		return nil, errors.New("dense layer dimensions must be positive")
	}
	return newLayer(layerDense, []int{inputSize, outputSize}, 0, false)
}

func NewReLULayer() (*Layer, error)    { return newLayer(layerReLU, nil, 0, false) }
func NewSigmoidLayer() (*Layer, error) { return newLayer(layerSigmoid, nil, 0, false) }
func NewTanhLayer() (*Layer, error)    { return newLayer(layerTanh, nil, 0, false) }
func NewSoftmaxLayer() (*Layer, error) { return newLayer(layerSoftmax, nil, 0, false) }

func NewDropoutLayer(rate float64) (*Layer, error) {
	if math.IsNaN(rate) || math.IsInf(rate, 0) || rate < 0 || rate >= 1 {
		return nil, errors.New("dropout rate must be finite and in [0, 1)")
	}
	return newLayer(layerDropout, nil, rate, false)
}

func NewBatchNormLayer(features int) (*Layer, error) {
	if features <= 0 {
		return nil, errors.New("batch-normalization feature count must be positive")
	}
	return newLayer(layerBatchNorm, []int{features}, 0, false)
}

func NewLayerNormLayer(features int) (*Layer, error) {
	if features <= 0 {
		return nil, errors.New("layer-normalization feature count must be positive")
	}
	return newLayer(layerNorm, []int{features}, 0, false)
}

func NewFlattenLayer() (*Layer, error) { return newLayer(layerFlatten, nil, 0, false) }

func NewConv1DLayer(inputChannels, outputChannels, kernelSize int) (*Layer, error) {
	if inputChannels <= 0 || outputChannels <= 0 || kernelSize <= 0 {
		return nil, errors.New("convolution dimensions must be positive")
	}
	return newLayer(layerConv1D, []int{inputChannels, outputChannels, kernelSize}, 0, false)
}

func NewConv2DLayer(inputChannels, outputChannels, kernelHeight, kernelWidth, strideHeight, strideWidth int) (*Layer, error) {
	if inputChannels <= 0 || outputChannels <= 0 || kernelHeight <= 0 || kernelWidth <= 0 ||
		strideHeight <= 0 || strideWidth <= 0 {
		return nil, errors.New("convolution dimensions and strides must be positive")
	}
	return newLayer(layerConv2D, []int{inputChannels, outputChannels, kernelHeight, kernelWidth, strideHeight, strideWidth}, 0, false)
}

func NewMaxPool2DLayer(poolHeight, poolWidth, strideHeight, strideWidth int) (*Layer, error) {
	if poolHeight <= 0 || poolWidth <= 0 || strideHeight < 0 || strideWidth < 0 {
		return nil, errors.New("pool dimensions must be positive and strides non-negative")
	}
	return newLayer(layerMaxPool2D, []int{poolHeight, poolWidth, strideHeight, strideWidth}, 0, false)
}

func NewAvgPool2DLayer(poolHeight, poolWidth, strideHeight, strideWidth int) (*Layer, error) {
	if poolHeight <= 0 || poolWidth <= 0 || strideHeight < 0 || strideWidth < 0 {
		return nil, errors.New("pool dimensions must be positive and strides non-negative")
	}
	return newLayer(layerAvgPool2D, []int{poolHeight, poolWidth, strideHeight, strideWidth}, 0, false)
}

func NewRNNLayer(inputSize, hiddenSize int, returnSequences bool) (*Layer, error) {
	if inputSize <= 0 || hiddenSize <= 0 {
		return nil, errors.New("recurrent layer dimensions must be positive")
	}
	return newLayer(layerRNN, []int{inputSize, hiddenSize}, 0, returnSequences)
}

func NewLSTMLayer(inputSize, hiddenSize int, returnSequences bool) (*Layer, error) {
	if inputSize <= 0 || hiddenSize <= 0 {
		return nil, errors.New("recurrent layer dimensions must be positive")
	}
	return newLayer(layerLSTM, []int{inputSize, hiddenSize}, 0, returnSequences)
}

func NewGRULayer(inputSize, hiddenSize int, returnSequences bool) (*Layer, error) {
	if inputSize <= 0 || hiddenSize <= 0 {
		return nil, errors.New("recurrent layer dimensions must be positive")
	}
	return newLayer(layerGRU, []int{inputSize, hiddenSize}, 0, returnSequences)
}

func NewEmbeddingLayer(vocabularySize, embeddingSize int) (*Layer, error) {
	if vocabularySize <= 0 || embeddingSize <= 0 {
		return nil, errors.New("embedding dimensions must be positive")
	}
	return newLayer(layerEmbedding, []int{vocabularySize, embeddingSize}, 0, false)
}

func (l *Layer) valid() error {
	if l == nil || l.handle == nil {
		return errors.New("layer is closed")
	}
	return nil
}

func (l *Layer) Close() error {
	if l != nil && l.handle != nil {
		C.coolbox_dl_layer_free(l.handle)
		l.handle = nil
	}
	return nil
}

func (l *Layer) Forward(input *Tensor) (*Tensor, error) {
	if err := l.valid(); err != nil {
		return nil, err
	}
	if err := input.valid(); err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_layer_forward(l.handle, input.handle, &nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

func (l *Layer) Backward(gradient *Tensor) (*Tensor, error) {
	if err := l.valid(); err != nil {
		return nil, err
	}
	if err := gradient.valid(); err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_layer_backward(l.handle, gradient.handle, &nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

func (l *Layer) UpdateParameters(learningRate float64) error {
	if err := l.valid(); err != nil {
		return err
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_layer_update(l.handle, C.double(learningRate), &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (l *Layer) SetTraining(training bool) error {
	if err := l.valid(); err != nil {
		return err
	}
	trainingFlag := C.int(0)
	if training {
		trainingFlag = 1
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_layer_set_training(l.handle, trainingFlag, &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (l *Layer) HasParameters() (bool, error) {
	if err := l.valid(); err != nil {
		return false, err
	}
	return C.coolbox_dl_layer_has_parameters(l.handle) != 0, nil
}

func (l *Layer) Name() (string, error) {
	if err := l.valid(); err != nil {
		return "", err
	}
	var name [128]C.char
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_layer_name(l.handle, &name[0], C.size_t(len(name)),
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return "", dlNativeError(&nativeErr)
	}
	return C.GoString(&name[0]), nil
}

type LossKind C.int

const (
	MeanSquaredError LossKind = iota
	BinaryCrossEntropy
	CategoricalCrossEntropy
)

// Loss wraps one of the native loss functions.
type Loss struct {
	handle unsafe.Pointer
}

func NewLoss(kind LossKind) (*Loss, error) {
	if kind < MeanSquaredError || kind > CategoricalCrossEntropy {
		return nil, errors.New("unknown loss kind")
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_loss_new(C.int(kind), &nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, dlNativeError(&nativeErr)
	}
	return &Loss{handle: handle}, nil
}

func (l *Loss) valid() error {
	if l == nil || l.handle == nil {
		return errors.New("loss is closed")
	}
	return nil
}

func (l *Loss) Close() error {
	if l != nil && l.handle != nil {
		C.coolbox_dl_loss_free(l.handle)
		l.handle = nil
	}
	return nil
}

func (l *Loss) Compute(predictions, targets *Tensor) (float64, error) {
	if err := l.valid(); err != nil {
		return 0, err
	}
	if err := predictions.valid(); err != nil {
		return 0, err
	}
	if err := targets.valid(); err != nil {
		return 0, err
	}
	var value C.double
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_loss_compute(l.handle, predictions.handle, targets.handle, &value,
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return 0, dlNativeError(&nativeErr)
	}
	return float64(value), nil
}

func (l *Loss) Gradient(predictions, targets *Tensor) (*Tensor, error) {
	if err := l.valid(); err != nil {
		return nil, err
	}
	if err := predictions.valid(); err != nil {
		return nil, err
	}
	if err := targets.valid(); err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_loss_gradient(l.handle, predictions.handle, targets.handle,
		&nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

type OptimizerKind C.int

const (
	StochasticGradientDescent OptimizerKind = iota
	AdamOptimizer
	RMSpropOptimizer
)

// Optimizer wraps a native parameter optimizer.
type Optimizer struct {
	handle unsafe.Pointer
}

func NewSGD(learningRate, momentum float64) (*Optimizer, error) {
	if !dlFinite(learningRate) || !dlFinite(momentum) || learningRate <= 0 || momentum < 0 || momentum >= 1 {
		return nil, errors.New("SGD requires a positive finite learning rate and momentum in [0, 1)")
	}
	return newOptimizer(StochasticGradientDescent, learningRate, momentum, 0)
}

func NewAdam(learningRate, beta1, beta2 float64) (*Optimizer, error) {
	if !dlFinite(learningRate) || !dlFinite(beta1) || !dlFinite(beta2) ||
		learningRate <= 0 || beta1 < 0 || beta1 >= 1 || beta2 < 0 || beta2 >= 1 {
		return nil, errors.New("Adam requires a positive finite learning rate and beta values in [0, 1)")
	}
	return newOptimizer(AdamOptimizer, learningRate, beta1, beta2)
}

func NewRMSprop(learningRate, decay float64) (*Optimizer, error) {
	if !dlFinite(learningRate) || !dlFinite(decay) || learningRate <= 0 || decay < 0 || decay >= 1 {
		return nil, errors.New("RMSprop requires a positive finite learning rate and decay in [0, 1)")
	}
	return newOptimizer(RMSpropOptimizer, learningRate, decay, 0)
}

func dlFinite(value float64) bool {
	return !math.IsNaN(value) && !math.IsInf(value, 0)
}

func newOptimizer(kind OptimizerKind, learningRate, parameter1, parameter2 float64) (*Optimizer, error) {
	if kind < StochasticGradientDescent || kind > RMSpropOptimizer {
		return nil, errors.New("unknown optimizer kind")
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_optimizer_new(C.int(kind), C.double(learningRate), C.double(parameter1),
		C.double(parameter2), &nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, dlNativeError(&nativeErr)
	}
	return &Optimizer{handle: handle}, nil
}

func (o *Optimizer) valid() error {
	if o == nil || o.handle == nil {
		return errors.New("optimizer is closed")
	}
	return nil
}

func (o *Optimizer) Close() error {
	if o != nil && o.handle != nil {
		C.coolbox_dl_optimizer_free(o.handle)
		o.handle = nil
	}
	return nil
}

func (o *Optimizer) Step(parameters, gradients *Tensor) error {
	if err := o.valid(); err != nil {
		return err
	}
	if err := parameters.valid(); err != nil {
		return err
	}
	if err := gradients.valid(); err != nil {
		return err
	}
	parameterShape, err := parameters.Shape()
	if err != nil {
		return err
	}
	gradientShape, err := gradients.Shape()
	if err != nil {
		return err
	}
	if len(parameterShape) != len(gradientShape) {
		return errors.New("parameter and gradient shapes must match")
	}
	for i := range parameterShape {
		if parameterShape[i] != gradientShape[i] {
			return errors.New("parameter and gradient shapes must match")
		}
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_optimizer_step(o.handle, parameters.handle, gradients.handle,
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (o *Optimizer) Reset() error {
	if err := o.valid(); err != nil {
		return err
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_optimizer_reset(o.handle, &nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (o *Optimizer) Name() (string, error) {
	if err := o.valid(); err != nil {
		return "", err
	}
	var name [128]C.char
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_optimizer_name(o.handle, &name[0], C.size_t(len(name)),
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return "", dlNativeError(&nativeErr)
	}
	return C.GoString(&name[0]), nil
}

// NeuralNetwork is a sequential native neural network.
type NeuralNetwork struct {
	handle  unsafe.Pointer
	hasLoss bool
}

func NewNeuralNetwork() (*NeuralNetwork, error) {
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_network_new(&nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, dlNativeError(&nativeErr)
	}
	return &NeuralNetwork{handle: handle}, nil
}

func (n *NeuralNetwork) valid() error {
	if n == nil || n.handle == nil {
		return errors.New("neural network is closed")
	}
	return nil
}

func (n *NeuralNetwork) Close() error {
	if n != nil && n.handle != nil {
		C.coolbox_dl_network_free(n.handle)
		n.handle = nil
	}
	return nil
}

func (n *NeuralNetwork) AddLayer(layer *Layer) error {
	if err := n.valid(); err != nil {
		return err
	}
	if err := layer.valid(); err != nil {
		return err
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_network_add_layer(n.handle, layer.handle, &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (n *NeuralNetwork) SetLoss(loss *Loss) error {
	if err := n.valid(); err != nil {
		return err
	}
	if err := loss.valid(); err != nil {
		return err
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_network_set_loss(n.handle, loss.handle, &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	n.hasLoss = true
	return nil
}

func (n *NeuralNetwork) SetOptimizer(optimizer *Optimizer) error {
	if err := n.valid(); err != nil {
		return err
	}
	if err := optimizer.valid(); err != nil {
		return err
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_network_set_optimizer(n.handle, optimizer.handle, &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (n *NeuralNetwork) Predict(input *Tensor) (*Tensor, error) {
	if err := n.valid(); err != nil {
		return nil, err
	}
	if err := input.valid(); err != nil {
		return nil, err
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_network_predict(n.handle, input.handle, &nativeErr[0], C.size_t(len(nativeErr)))
	return tensorFromHandle(handle, &nativeErr)
}

func (n *NeuralNetwork) TrainStep(input, target *Tensor) (float64, error) {
	if err := n.valid(); err != nil {
		return 0, err
	}
	if !n.hasLoss {
		return 0, errors.New("neural network loss has not been set")
	}
	if err := input.valid(); err != nil {
		return 0, err
	}
	if err := target.valid(); err != nil {
		return 0, err
	}
	if err := n.SetTraining(true); err != nil {
		return 0, err
	}
	var value C.double
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_network_train_step(n.handle, input.handle, target.handle, &value,
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return 0, dlNativeError(&nativeErr)
	}
	return float64(value), nil
}

func cHandleArray(values []*Tensor) (unsafe.Pointer, error) {
	if len(values) == 0 {
		return nil, errors.New("tensor list must not be empty")
	}
	bytes := C.size_t(len(values)) * C.size_t(unsafe.Sizeof(uintptr(0)))
	array := C.malloc(bytes)
	if array == nil {
		return nil, errors.New("failed to allocate native tensor list")
	}
	handles := unsafe.Slice((*unsafe.Pointer)(array), len(values))
	for i, value := range values {
		if err := value.valid(); err != nil {
			C.free(array)
			return nil, fmt.Errorf("tensor %d: %w", i, err)
		}
		handles[i] = value.handle
	}
	return array, nil
}

func (n *NeuralNetwork) Evaluate(inputs, targets []*Tensor) (float64, error) {
	if err := n.valid(); err != nil {
		return 0, err
	}
	if !n.hasLoss {
		return 0, errors.New("neural network loss has not been set")
	}
	if len(inputs) == 0 || len(inputs) != len(targets) {
		return 0, errors.New("evaluation inputs and targets must have the same non-zero length")
	}
	nativeInputs, err := cHandleArray(inputs)
	if err != nil {
		return 0, err
	}
	defer C.free(nativeInputs)
	nativeTargets, err := cHandleArray(targets)
	if err != nil {
		return 0, err
	}
	defer C.free(nativeTargets)
	var value C.double
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_network_evaluate(n.handle, (*unsafe.Pointer)(nativeInputs), (*unsafe.Pointer)(nativeTargets),
		C.size_t(len(inputs)), &value, &nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return 0, dlNativeError(&nativeErr)
	}
	return float64(value), nil
}

func (n *NeuralNetwork) Train(inputs, targets []*Tensor, epochs, batchSize int, verbose bool) error {
	if err := n.valid(); err != nil {
		return err
	}
	if !n.hasLoss {
		return errors.New("neural network loss has not been set")
	}
	if len(inputs) == 0 || len(inputs) != len(targets) {
		return errors.New("training inputs and targets must have the same non-zero length")
	}
	if epochs < 0 {
		return errors.New("epochs must not be negative")
	}
	if epochs > math.MaxInt32 || batchSize > math.MaxInt32 {
		return errors.New("epochs and batch size must fit in the native integer range")
	}
	if batchSize <= 0 {
		return errors.New("batch size must be positive")
	}
	if err := n.SetTraining(true); err != nil {
		return err
	}
	nativeInputs, err := cHandleArray(inputs)
	if err != nil {
		return err
	}
	defer C.free(nativeInputs)
	nativeTargets, err := cHandleArray(targets)
	if err != nil {
		return err
	}
	defer C.free(nativeTargets)
	verboseFlag := C.int(0)
	if verbose {
		verboseFlag = 1
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_network_train(n.handle, (*unsafe.Pointer)(nativeInputs), (*unsafe.Pointer)(nativeTargets),
		C.size_t(len(inputs)), C.int(epochs), C.int(batchSize), verboseFlag,
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (n *NeuralNetwork) SetTraining(training bool) error {
	if err := n.valid(); err != nil {
		return err
	}
	trainingFlag := C.int(0)
	if training {
		trainingFlag = 1
	}
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_network_set_training(n.handle, trainingFlag, &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (n *NeuralNetwork) LayerCount() (int, error) {
	if err := n.valid(); err != nil {
		return 0, err
	}
	return int(C.coolbox_dl_network_layer_count(n.handle)), nil
}

type FeedbackStyle int

const (
	RewardOnlyFeedback FeedbackStyle = iota
	RichLanguageFeedback
)

// Helix exposes the finite-hypothesis HELiX learner from the DL agent package.
type Helix struct {
	handle        unsafe.Pointer
	numActions    int
	feedbackStyle FeedbackStyle
}

func NewHelix(numActions, truth int, style FeedbackStyle) (*Helix, error) {
	if numActions <= 0 {
		return nil, errors.New("number of actions must be positive")
	}
	if truth < 0 || truth >= numActions {
		return nil, errors.New("truth index is outside the action range")
	}
	if style != RewardOnlyFeedback && style != RichLanguageFeedback {
		return nil, errors.New("unknown feedback style")
	}
	rich := C.int(0)
	if style == RichLanguageFeedback {
		rich = 1
	}
	var nativeErr [nativeErrorCapacity]C.char
	handle := C.coolbox_dl_helix_new(C.size_t(numActions), C.size_t(truth), rich,
		&nativeErr[0], C.size_t(len(nativeErr)))
	if handle == nil {
		return nil, dlNativeError(&nativeErr)
	}
	return &Helix{handle: handle, numActions: numActions, feedbackStyle: style}, nil
}

func (h *Helix) valid() error {
	if h == nil || h.handle == nil {
		return errors.New("HELiX learner is closed")
	}
	return nil
}

func (h *Helix) Close() error {
	if h != nil && h.handle != nil {
		C.coolbox_dl_helix_free(h.handle)
		h.handle = nil
	}
	return nil
}

func (h *Helix) SelectAction() (int, error) {
	if err := h.valid(); err != nil {
		return 0, err
	}
	var action C.size_t
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_helix_select_action(h.handle, &action, &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return 0, dlNativeError(&nativeErr)
	}
	return int(action), nil
}

func (h *Helix) Observe(action int, feedback string) error {
	if err := h.valid(); err != nil {
		return err
	}
	if action < 0 || action >= h.numActions {
		return errors.New("action index is outside the action range")
	}
	if strings.IndexByte(feedback, 0) >= 0 {
		return errors.New("feedback must not contain a NUL byte")
	}
	nativeFeedback := C.CString(feedback)
	defer C.free(unsafe.Pointer(nativeFeedback))
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_helix_observe(h.handle, C.size_t(action), nativeFeedback,
		&nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return dlNativeError(&nativeErr)
	}
	return nil
}

func (h *Helix) Interact() (int, error) {
	if err := h.valid(); err != nil {
		return 0, err
	}
	var action C.size_t
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_helix_interact(h.handle, &action, &nativeErr[0],
		C.size_t(len(nativeErr))) == 0 {
		return 0, dlNativeError(&nativeErr)
	}
	return int(action), nil
}

func (h *Helix) Rounds() (int, error) {
	if err := h.valid(); err != nil {
		return 0, err
	}
	return int(C.coolbox_dl_helix_rounds(h.handle)), nil
}

func (h *Helix) Mistakes() (int, error) {
	if err := h.valid(); err != nil {
		return 0, err
	}
	return int(C.coolbox_dl_helix_mistakes(h.handle)), nil
}

func (h *Helix) CumulativeRegret() (float64, error) {
	if err := h.valid(); err != nil {
		return 0, err
	}
	return float64(C.coolbox_dl_helix_cumulative_regret(h.handle)), nil
}

func (h *Helix) Identified() (bool, error) {
	if err := h.valid(); err != nil {
		return false, err
	}
	return C.coolbox_dl_helix_identified(h.handle) != 0, nil
}

func (h *Helix) VersionSpace() ([]int, error) {
	if err := h.valid(); err != nil {
		return nil, err
	}
	indices := make([]C.size_t, h.numActions)
	count := int(C.coolbox_dl_helix_version_space(h.handle, sizePointer(indices), C.size_t(len(indices))))
	result := make([]int, count)
	for i, index := range indices[:count] {
		result[i] = int(index)
	}
	return result, nil
}

func TransferEluderDimension(numActions, truth int, style FeedbackStyle, epsilon float64, maxActions int) (int, error) {
	if numActions <= 0 || truth < 0 || truth >= numActions {
		return 0, errors.New("action count and truth index are invalid")
	}
	if maxActions <= 0 {
		return 0, errors.New("maximum enumerated actions must be positive")
	}
	if !dlFinite(epsilon) || epsilon < 0 {
		return 0, errors.New("epsilon must be finite and non-negative")
	}
	if style != RewardOnlyFeedback && style != RichLanguageFeedback {
		return 0, errors.New("unknown feedback style")
	}
	rich := C.int(0)
	if style == RichLanguageFeedback {
		rich = 1
	}
	var dimension C.size_t
	var nativeErr [nativeErrorCapacity]C.char
	if C.coolbox_dl_helix_dimension(C.size_t(numActions), C.size_t(truth), rich, C.double(epsilon),
		C.size_t(maxActions), &dimension, &nativeErr[0], C.size_t(len(nativeErr))) == 0 {
		return 0, dlNativeError(&nativeErr)
	}
	return int(dimension), nil
}
