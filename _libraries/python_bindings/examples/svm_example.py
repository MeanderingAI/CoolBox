#!/usr/bin/env python3
"""
Example usage of the Support Vector Machine Python bindings
"""

import numpy as np
import ml_core

def test_svm():
    print("Testing Support Vector Machine...")
    
    # Create sample data using numpy
    X = np.array([
        [1.0, 2.0],
        [2.0, 3.0],
        [3.0, 1.0],
        [6.0, 5.0],
        [7.0, 7.0],
        [8.0, 6.0]
    ])
    
    y = np.array([-1.0, -1.0, -1.0, 1.0, 1.0, 1.0])
    
    # Create different kernels
    print("\n1. Testing Linear Kernel SVM:")
    linear_kernel = ml_core.svm.LinearKernel()
    svm_linear = ml_core.svm.SVM(linear_kernel)
    svm_linear.fit(X, y)
    
    # Make predictions
    test_samples = [
        np.array([2.0, 2.0]),
        np.array([6.0, 6.0])
    ]
    
    for sample in test_samples:
        prediction = svm_linear.predict(sample)
        print(f"Linear SVM - Sample {sample}: Predicted = {prediction}")
    
    print("\n2. Testing RBF Kernel SVM:")
    rbf_kernel = ml_core.svm.RBFKernel(gamma=0.5)
    svm_rbf = ml_core.svm.SVM(rbf_kernel)
    svm_rbf.fit(X, y)
    
    for sample in test_samples:
        prediction = svm_rbf.predict(sample)
        print(f"RBF SVM - Sample {sample}: Predicted = {prediction}")

if __name__ == "__main__":
    test_svm()