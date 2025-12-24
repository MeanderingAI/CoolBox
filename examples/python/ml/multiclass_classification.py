"""
Deep Learning Library - MNIST-style Classification Example
Demonstrates multi-class classification with softmax output
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'python_bindings', 'build'))

try:
    from ml_core import deep_learning as dl
    import random
    
    print("Deep Learning Library - Multi-Class Classification")
    print("=" * 60)
    
    # Create synthetic 3-class dataset
    # Class 0: points around (0, 0)
    # Class 1: points around (1, 1)
    # Class 2: points around (-1, 1)
    
    random.seed(42)
    
    def generate_data(n_samples_per_class=30):
        inputs = []
        targets = []
        
        centers = [(0, 0), (1, 1), (-1, 1)]
        
        for class_idx, (cx, cy) in enumerate(centers):
            for _ in range(n_samples_per_class):
                # Add some noise
                x = cx + random.gauss(0, 0.3)
                y = cy + random.gauss(0, 0.3)
                
                inputs.append(dl.Tensor([1, 2], [x, y]))
                
                # One-hot encoding
                target = [0.0, 0.0, 0.0]
                target[class_idx] = 1.0
                targets.append(dl.Tensor([1, 3], target))
        
        return inputs, targets
    
    # Generate training data
    train_inputs, train_targets = generate_data(30)
    
    print(f"Generated {len(train_inputs)} training samples across 3 classes")
    
    # Create neural network for multi-class classification
    nn = dl.NeuralNetwork()
    
    # Architecture
    nn.add_layer(dl.DenseLayer(2, 16))   # Input layer
    nn.add_layer(dl.ReLULayer())
    nn.add_layer(dl.DenseLayer(16, 8))
    nn.add_layer(dl.ReLULayer())
    nn.add_layer(dl.DenseLayer(8, 3))    # Output layer (3 classes)
    nn.add_layer(dl.SoftmaxLayer())      # Softmax for probabilities
    
    # Use categorical cross-entropy for multi-class
    nn.set_loss(dl.CategoricalCrossEntropyLoss())
    
    print("\nNetwork Architecture:")
    nn.summary()
    
    # Train
    print("\nTraining...")
    nn.train(train_inputs, train_targets, epochs=100, batch_size=16, verbose=True)
    
    # Test predictions
    print("\nTest Predictions:")
    test_points = [
        (0.0, 0.0, "Class 0 region"),
        (1.0, 1.0, "Class 1 region"),
        (-1.0, 1.0, "Class 2 region"),
        (0.5, 0.5, "Between 0 and 1"),
        (-0.5, 0.5, "Between 0 and 2")
    ]
    
    for x, y, description in test_points:
        test_input = dl.Tensor([1, 2], [x, y])
        output = nn.predict(test_input)
        probs = output.data()
        
        predicted_class = probs.index(max(probs))
        
        print(f"\nPoint ({x:5.2f}, {y:5.2f}) - {description}")
        print(f"  Class 0 prob: {probs[0]:.4f}")
        print(f"  Class 1 prob: {probs[1]:.4f}")
        print(f"  Class 2 prob: {probs[2]:.4f}")
        print(f"  Predicted: Class {predicted_class}")
    
    # Evaluate on training data
    print("\n" + "=" * 60)
    train_loss = nn.evaluate(train_inputs, train_targets)
    print(f"Training Loss: {train_loss:.6f}")
    
except ImportError as e:
    print(f"Error: Could not import ml_core module: {e}")
    print("Please build the Python bindings first:")
    print("  cd python_bindings")
    print("  ./build.sh")
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
