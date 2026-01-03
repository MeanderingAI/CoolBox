# Deep Learning Library

A lightweight deep learning library implemented in C++ with Python bindings, designed for educational purposes and simple neural network applications.

## Features

### Core Components

- **Tensor**: Multi-dimensional array operations with support for:
  - Element-wise operations (+, -, *, /)
  - Matrix multiplication
  - Reshape and transpose
  - Random initialization

- **Layers**:
  - `DenseLayer`: Fully connected layer with Xavier/Glorot initialization
  - `ReLULayer`: Rectified Linear Unit activation
  - `SigmoidLayer`: Sigmoid activation
  - `TanhLayer`: Hyperbolic tangent activation
  - `SoftmaxLayer`: Softmax activation for multi-class classification
  - `DropoutLayer`: Regularization layer with configurable dropout rate

- **Loss Functions**:
  - `MSELoss`: Mean Squared Error for regression
  - `BCELoss`: Binary Cross-Entropy for binary classification
  - `CategoricalCrossEntropyLoss`: For multi-class classification

- **Optimizers**:
  - `SGD`: Stochastic Gradient Descent with momentum
  - `Adam`: Adaptive Moment Estimation
  - `RMSprop`: Root Mean Square Propagation

- **Neural Network**: High-level API for building and training networks

## Building

### C++ Library

```bash
mkdir build && cd build
cmake ..
make
```

### Python Bindings

```bash
cd python_bindings
./build.sh
```

## Usage

### C++ Example

```cpp
#include "deep_learning/neural_network.h"
#include "deep_learning/layer.h"
#include "deep_learning/loss.h"

using namespace ml::deep_learning;

int main() {
    // Create dataset
    std::vector<Tensor> inputs;
    std::vector<Tensor> targets;
    
    inputs.push_back(Tensor({1, 2}, {0.0, 0.0}));
    targets.push_back(Tensor({1, 1}, {0.0}));
    
    // Create network
    NeuralNetwork nn;
    nn.add_layer(std::make_shared<DenseLayer>(2, 4));
    nn.add_layer(std::make_shared<ReLULayer>());
    nn.add_layer(std::make_shared<DenseLayer>(4, 1));
    nn.add_layer(std::make_shared<SigmoidLayer>());
    
    // Set loss
    nn.set_loss(std::make_shared<MSELoss>());
    
    // Train
    nn.train(inputs, targets, epochs=1000, batch_size=4);
    
    // Predict
    Tensor output = nn.predict(inputs[0]);
    
    return 0;
}
```

### Python Example

```python
from ml_core import deep_learning as dl

# Create dataset
inputs = [
    dl.Tensor([1, 2], [0.0, 0.0]),
    dl.Tensor([1, 2], [0.0, 1.0]),
    dl.Tensor([1, 2], [1.0, 0.0]),
    dl.Tensor([1, 2], [1.0, 1.0])
]

targets = [
    dl.Tensor([1, 1], [0.0]),
    dl.Tensor([1, 1], [1.0]),
    dl.Tensor([1, 1], [1.0]),
    dl.Tensor([1, 1], [0.0])
]

# Create network
nn = dl.NeuralNetwork()
nn.add_layer(dl.DenseLayer(2, 4))
nn.add_layer(dl.ReLULayer())
nn.add_layer(dl.DenseLayer(4, 1))
nn.add_layer(dl.SigmoidLayer())

# Set loss
nn.set_loss(dl.MSELoss())

# Train
nn.train(inputs, targets, epochs=1000, batch_size=4)

# Predict
output = nn.predict(inputs[0])
print(f"Prediction: {output.data()[0]}")
```

## Examples

The `examples/` directory contains several demonstrations:

1. **deep_learning_example.py**: XOR problem and binary classification
2. **multiclass_classification.py**: 3-class classification with softmax
3. **deep_learning_example.cpp**: C++ XOR example

Run the Python examples:

```bash
cd examples
python deep_learning_example.py
python multiclass_classification.py
```

## Architecture

### Tensor Shape Convention

- Tensors use shape notation: `[batch_size, features]`
- For single samples: `[1, features]`
- For batches: `[batch_size, features]`

### Forward and Backward Pass

Each layer implements:
- `forward()`: Computes output from input
- `backward()`: Computes gradient w.r.t. input from gradient w.r.t. output

The network automatically manages the backward pass through all layers.

## Supported Operations

### Tensor Operations

- Element-wise: `add`, `subtract`, `multiply`, `divide`
- Matrix operations: `matmul`, `transpose`
- Utilities: `reshape`, `fill`, `randomize`, `clone`

### Training Features

- Mini-batch training with shuffling
- Automatic gradient computation via backpropagation
- Multiple optimizer support
- Training/evaluation mode (for dropout)

## Limitations

This is a basic implementation designed for learning and simple applications:

- No GPU acceleration
- Limited optimization (no parallelization)
- Basic tensor operations only
- No convolutional or recurrent layers
- No automatic differentiation framework
- Simple batching (processes samples individually)

## Testing

Run the test suite:

```bash
./runTestSuite.sh
```

The test suite includes:
- Tensor operations
- Layer forward/backward passes
- Loss functions
- XOR problem convergence test

## Future Enhancements

Potential improvements:
- Batch normalization layer
- Convolutional layers
- Recurrent layers (LSTM, GRU)
- More optimizers (AdaGrad, AdaDelta)
- Learning rate schedulers
- Model serialization/deserialization
- GPU acceleration via CUDA
- Automatic differentiation

## License

See LICENSE file in the root directory.

## Contributing

This library is part of a larger ML toolkit. See the main README.md for contribution guidelines.
