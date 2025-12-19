"""
Deep Learning Library - Python Example
Train a simple neural network on the XOR problem
"""

import sys
import os

# Add the build directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'python_bindings', 'build'))

try:
    from ml_core import deep_learning as dl
    
    print("Deep Learning Library - Python XOR Example")
    print("=" * 50)
    
    # Create XOR dataset
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
    
    # Create neural network
    nn = dl.NeuralNetwork()
    
    # Add layers
    nn.add_layer(dl.DenseLayer(2, 4))   # Input to hidden layer
    nn.add_layer(dl.ReLULayer())        # Activation
    nn.add_layer(dl.DenseLayer(4, 1))   # Hidden to output layer
    nn.add_layer(dl.SigmoidLayer())     # Output activation
    
    # Set loss function
    nn.set_loss(dl.MSELoss())
    
    # Display network architecture
    print("\nNetwork Architecture:")
    nn.summary()
    
    # Train the network
    print("\nTraining...")
    nn.train(inputs, targets, epochs=1000, batch_size=4, verbose=True)
    
    # Test the network
    print("\nTesting:")
    for i in range(len(inputs)):
        output = nn.predict(inputs[i])
        input_data = inputs[i].data()
        print(f"Input: [{input_data[0]:.1f}, {input_data[1]:.1f}] -> "
              f"Output: {output.data()[0]:.4f} (Target: {targets[i].data()[0]:.1f})")
    
    print("\n" + "=" * 50)
    print("Advanced Example: Binary Classification")
    print("=" * 50)
    
    # Create a more complex dataset
    import random
    random.seed(42)
    
    n_samples = 100
    train_inputs = []
    train_targets = []
    
    for _ in range(n_samples):
        x1 = random.uniform(-1, 1)
        x2 = random.uniform(-1, 1)
        # Simple decision boundary: x1 + x2 > 0
        label = 1.0 if (x1 + x2) > 0 else 0.0
        
        train_inputs.append(dl.Tensor([1, 2], [x1, x2]))
        train_targets.append(dl.Tensor([1, 1], [label]))
    
    # Create a new network with dropout
    nn2 = dl.NeuralNetwork()
    nn2.add_layer(dl.DenseLayer(2, 8))
    nn2.add_layer(dl.ReLULayer())
    nn2.add_layer(dl.DropoutLayer(0.2))
    nn2.add_layer(dl.DenseLayer(8, 4))
    nn2.add_layer(dl.ReLULayer())
    nn2.add_layer(dl.DenseLayer(4, 1))
    nn2.add_layer(dl.SigmoidLayer())
    
    nn2.set_loss(dl.BCELoss())
    
    print("\nNetwork Architecture (with Dropout):")
    nn2.summary()
    
    print("\nTraining on 100 samples...")
    nn2.train(train_inputs, train_targets, epochs=50, batch_size=16, verbose=True)
    
    # Evaluate
    print("\nEvaluation:")
    loss = nn2.evaluate(train_inputs, train_targets)
    print(f"Final Loss: {loss:.6f}")
    
    # Test on a few examples
    print("\nSample Predictions:")
    test_cases = [
        (0.5, 0.5),
        (-0.5, -0.5),
        (0.5, -0.3),
        (-0.3, 0.5)
    ]
    
    for x1, x2 in test_cases:
        test_input = dl.Tensor([1, 2], [x1, x2])
        output = nn2.predict(test_input)
        expected = 1.0 if (x1 + x2) > 0 else 0.0
        print(f"Input: [{x1:5.2f}, {x2:5.2f}] -> "
              f"Output: {output.data()[0]:.4f} (Expected: {expected:.1f})")
    
except ImportError as e:
    print(f"Error: Could not import ml_core module: {e}")
    print("Please build the Python bindings first:")
    print("  cd python_bindings")
    print("  ./build.sh")
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
