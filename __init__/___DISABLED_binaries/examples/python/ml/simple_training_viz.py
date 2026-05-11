#!/usr/bin/env python3
"""
Simple Neural Network Training with Visualization

Shows training progress for regression and classification tasks.
"""

import sys
sys.path.insert(0, './build')
import ml_core
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

plt.style.use('seaborn-v0_8-darkgrid')

def train_with_monitoring(net, X_train, y_train, X_test, y_test, epochs=100, print_every=20):
    """Train network and return loss history"""
    train_losses = []
    test_losses = []
    
    for epoch in range(epochs):
        # Create tensors for this batch
        X_tensor = ml_core.deep_learning.Tensor([len(X_train), X_train.shape[1]])
        y_tensor = ml_core.deep_learning.Tensor([len(y_train), y_train.shape[1]])
        
        # Fill tensors
        X_data = X_tensor.data()
        y_data = y_tensor.data()
        for i in range(len(X_train)):
            for j in range(X_train.shape[1]):
                X_data[i * X_train.shape[1] + j] = X_train[i, j]
        for i in range(len(y_train)):
            for j in range(y_train.shape[1]):
                y_data[i * y_train.shape[1] + j] = y_train[i, j]
        
        # Train
        train_loss = net.train_step(X_tensor, y_tensor)
        train_losses.append(train_loss)
        
        # Evaluate on test set
        X_test_tensor = ml_core.deep_learning.Tensor([len(X_test), X_test.shape[1]])
        X_test_data = X_test_tensor.data()
        for i in range(len(X_test)):
            for j in range(X_test.shape[1]):
                X_test_data[i * X_test.shape[1] + j] = X_test[i, j]
        
        predictions = net.predict(X_test_tensor)
        pred_data = predictions.data()
        pred_array = np.array([pred_data[i] for i in range(len(pred_data))]).reshape(X_test.shape[0], y_test.shape[1])
        
        test_loss = np.mean((pred_array - y_test) ** 2)
        test_losses.append(test_loss)
        
        if (epoch + 1) % print_every == 0:
            print(f"  Epoch {epoch+1}/{epochs} - Train Loss: {train_loss:.4f}, Test Loss: {test_loss:.4f}")
    
    return train_losses, test_losses

def example_regression():
    """Regression example with visualization"""
    print("=" * 70)
    print("Regression Training with Visualization")
    print("=" * 70)
    
    # Generate data
    np.random.seed(42)
    X_train = np.linspace(-3, 3, 100).reshape(-1, 1)
    y_train = 0.5 * X_train**2 + 2 * np.sin(X_train) + np.random.randn(100, 1) * 0.3
    
    X_test = np.linspace(-3, 3, 50).reshape(-1, 1)
    y_test = 0.5 * X_test**2 + 2 * np.sin(X_test)
    
    print(f"Training samples: {len(X_train)}, Test samples: {len(X_test)}")
    
    # Create network
    net = ml_core.deep_learning.NeuralNetwork()
    net.add_layer(ml_core.deep_learning.DenseLayer(1, 20))
    net.add_layer(ml_core.deep_learning.ReLULayer())
    net.add_layer(ml_core.deep_learning.DenseLayer(20, 20))
    net.add_layer(ml_core.deep_learning.ReLULayer())
    net.add_layer(ml_core.deep_learning.DenseLayer(20, 1))
    
    net.set_loss(ml_core.deep_learning.MSELoss())
    net.set_optimizer(ml_core.deep_learning.Adam(0.01))
    
    print("\nTraining...")
    train_losses, test_losses = train_with_monitoring(net, X_train, y_train, X_test, y_test, epochs=150, print_every=30)
    
    # Get final predictions
    X_test_tensor = ml_core.deep_learning.Tensor([len(X_test), 1])
    X_test_data = X_test_tensor.data()
    for i in range(len(X_test)):
        X_test_data[i] = X_test[i, 0]
    
    predictions = net.predict(X_test_tensor)
    pred_data = predictions.data()
    pred_array = np.array([pred_data[i] for i in range(len(pred_data))]).reshape(-1, 1)
    
    # Visualize
    fig = plt.figure(figsize=(15, 5))
    
    # Plot 1: Loss curves
    ax1 = plt.subplot(1, 3, 1)
    ax1.plot(train_losses, label='Train Loss', linewidth=2, color='blue')
    ax1.plot(test_losses, label='Test Loss', linewidth=2, color='red')
    ax1.set_xlabel('Epoch', fontsize=12)
    ax1.set_ylabel('MSE Loss', fontsize=12)
    ax1.set_title('Training Progress', fontsize=14, fontweight='bold')
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Predictions
    ax2 = plt.subplot(1, 3, 2)
    ax2.scatter(X_train, y_train, alpha=0.4, s=30, label='Train Data', color='gray')
    ax2.plot(X_test, y_test, 'g-', label='True Function', linewidth=3, alpha=0.7)
    ax2.plot(X_test, pred_array, 'r--', label='Predictions', linewidth=2)
    ax2.set_xlabel('X', fontsize=12)
    ax2.set_ylabel('y', fontsize=12)
    ax2.set_title('Model Fit', fontsize=14, fontweight='bold')
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3)
    
    # Plot 3: Residuals
    ax3 = plt.subplot(1, 3, 3)
    residuals = y_test - pred_array
    ax3.scatter(pred_array, residuals, alpha=0.6, s=50, color='purple')
    ax3.axhline(y=0, color='r', linestyle='--', linewidth=2)
    ax3.set_xlabel('Predicted', fontsize=12)
    ax3.set_ylabel('Residual', fontsize=12)
    ax3.set_title('Residual Analysis', fontsize=14, fontweight='bold')
    ax3.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('regression_demo.png', dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: regression_demo.png")
    print(f"Final Train Loss: {train_losses[-1]:.4f}")
    print(f"Final Test Loss: {test_losses[-1]:.4f}\n")
    plt.show()

def example_classification():
    """Classification example"""
    print("=" * 70)
    print("Classification Training with Visualization")
    print("=" * 70)
    
    # Generate XOR-like data
    np.random.seed(42)
    n = 200
    X = np.random.randn(n, 2) * 2
    y = ((X[:, 0] > 0) ^ (X[:, 1] > 0)).astype(float).reshape(-1, 1)
    
    # Add noise
    noise = np.random.choice(n, size=int(n * 0.05), replace=False)
    y[noise] = 1 - y[noise]
    
    # Split
    split = int(0.8 * n)
    X_train, X_test = X[:split], X[split:]
    y_train, y_test = y[:split], y[split:]
    
    print(f"Training samples: {len(X_train)}, Test samples: {len(X_test)}")
    
    # Create network
    net = ml_core.deep_learning.NeuralNetwork()
    net.add_layer(ml_core.deep_learning.DenseLayer(2, 16))
    net.add_layer(ml_core.deep_learning.ReLULayer())
    net.add_layer(ml_core.deep_learning.DenseLayer(16, 8))
    net.add_layer(ml_core.deep_learning.ReLULayer())
    net.add_layer(ml_core.deep_learning.DenseLayer(8, 1))
    net.add_layer(ml_core.deep_learning.SigmoidLayer())
    
    net.set_loss(ml_core.deep_learning.BCELoss())
    net.set_optimizer(ml_core.deep_learning.Adam(0.01))
    
    print("\nTraining...")
    train_losses, test_losses = train_with_monitoring(net, X_train, y_train, X_test, y_test, epochs=200, print_every=40)
    
    # Calculate accuracies
    X_train_tensor = ml_core.deep_learning.Tensor([len(X_train), 2])
    X_train_data = X_train_tensor.data()
    for i in range(len(X_train)):
        for j in range(2):
            X_train_data[i * 2 + j] = X_train[i, j]
    
    train_pred = net.predict(X_train_tensor)
    train_pred_data = train_pred.data()
    train_pred_array = np.array([train_pred_data[i] for i in range(len(train_pred_data))]).reshape(-1, 1)
    train_acc = np.mean((train_pred_array > 0.5) == y_train)
    
    X_test_tensor = ml_core.deep_learning.Tensor([len(X_test), 2])
    X_test_data = X_test_tensor.data()
    for i in range(len(X_test)):
        for j in range(2):
            X_test_data[i * 2 + j] = X_test[i, j]
    
    test_pred = net.predict(X_test_tensor)
    test_pred_data = test_pred.data()
    test_pred_array = np.array([test_pred_data[i] for i in range(len(test_pred_data))]).reshape(-1, 1)
    test_acc = np.mean((test_pred_array > 0.5) == y_test)
    
    # Visualize
    fig = plt.figure(figsize=(15, 5))
    
    # Plot 1: Loss
    ax1 = plt.subplot(1, 3, 1)
    ax1.plot(train_losses, linewidth=2, color='darkblue', label='Train')
    ax1.plot(test_losses, linewidth=2, color='orange', label='Test')
    ax1.set_xlabel('Epoch', fontsize=12)
    ax1.set_ylabel('BCE Loss', fontsize=12)
    ax1.set_title('Training Loss', fontsize=14, fontweight='bold')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Decision boundary
    ax2 = plt.subplot(1, 3, 2)
    
    x_min, x_max = X[:, 0].min() - 1, X[:, 0].max() + 1
    y_min, y_max = X[:, 1].min() - 1, X[:, 1].max() + 1
    xx, yy = np.meshgrid(np.linspace(x_min, x_max, 100),
                         np.linspace(y_min, y_max, 100))
    
    mesh_input = np.c_[xx.ravel(), yy.ravel()]
    mesh_tensor = ml_core.deep_learning.Tensor([len(mesh_input), 2])
    mesh_data = mesh_tensor.data()
    for i in range(len(mesh_input)):
        for j in range(2):
            mesh_data[i * 2 + j] = mesh_input[i, j]
    
    Z_pred = net.predict(mesh_tensor)
    Z_data = Z_pred.data()
    Z = np.array([Z_data[i] for i in range(len(Z_data))]).reshape(xx.shape)
    
    contour = ax2.contourf(xx, yy, Z, levels=20, cmap='RdYlBu', alpha=0.6)
    ax2.contour(xx, yy, Z, levels=[0.5], colors='black', linewidths=2)
    
    ax2.scatter(X[y.flatten() == 0, 0], X[y.flatten() == 0, 1],
               c='blue', s=50, alpha=0.7, edgecolors='k', label='Class 0')
    ax2.scatter(X[y.flatten() == 1, 0], X[y.flatten() == 1, 1],
               c='red', s=50, alpha=0.7, edgecolors='k', label='Class 1')
    
    ax2.set_xlabel('Feature 1', fontsize=12)
    ax2.set_ylabel('Feature 2', fontsize=12)
    ax2.set_title('Decision Boundary', fontsize=14, fontweight='bold')
    ax2.legend()
    
    # Plot 3: Accuracy info
    ax3 = plt.subplot(1, 3, 3)
    ax3.axis('off')
    
    info_text = f"""
    Final Results
    ═══════════════════════
    
    Train Accuracy: {train_acc:.2%}
    Test Accuracy:  {test_acc:.2%}
    
    Final Train Loss: {train_losses[-1]:.4f}
    Final Test Loss:  {test_losses[-1]:.4f}
    
    Network Architecture:
    • Input: 2 features
    • Hidden: 16 → 8 neurons
    • Output: 1 (sigmoid)
    • Optimizer: Adam (lr=0.01)
    """
    
    ax3.text(0.1, 0.5, info_text, fontsize=11, family='monospace',
            verticalalignment='center')
    
    plt.tight_layout()
    plt.savefig('classification_demo.png', dpi=300, bbox_inches='tight')
    print(f"\n✓ Saved: classification_demo.png")
    print(f"Train Accuracy: {train_acc:.2%}")
    print(f"Test Accuracy: {test_acc:.2%}\n")
    plt.show()

if __name__ == "__main__":
    print("\n" + "=" * 70)
    print("NEURAL NETWORK TRAINING VISUALIZATION")
    print("=" * 70 + "\n")
    
    example_regression()
    example_classification()
    
    print("=" * 70)
    print("✓ All visualizations complete!")
    print("=" * 70)
