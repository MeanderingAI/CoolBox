#!/usr/bin/env python3
"""
Example usage of the Linear Regression Python bindings
"""

import numpy as np
import ml_core

def test_linear_regression():
    print("Testing Linear Regression...")
    
    # Create sample data
    X = [
        [1.0, 2.0],
        [2.0, 3.0], 
        [3.0, 4.0],
        [4.0, 5.0],
        [5.0, 6.0]
    ]
    
    # Target values (linear relationship: y = 2*x1 + 1.5*x2 + noise)
    y = [7.0, 9.5, 12.0, 14.5, 17.0]
    
    print("1. Testing Gradient Descent:")
    # Create fit method for gradient descent
    gd_method = ml_core.glm.LinearRegressionFitMethod(
        num_iterations=1000,
        learning_rate=0.01,
        type=ml_core.glm.LinearRegressionType.GRADIENT_DESCENT
    )
    
    # Create and train linear regression model
    lr_gd = ml_core.glm.LinearRegression(gd_method)
    lr_gd.fit(X, y)
    
    # Make predictions
    test_samples = [
        [2.5, 3.5],
        [6.0, 7.0]
    ]
    
    for sample in test_samples:
        prediction = lr_gd.predict(sample)
        print(f"GD - Sample {sample}: Predicted = {prediction:.3f}")
    
    # Get coefficients
    weights, bias = lr_gd.get_coefficients()
    print(f"GD - Learned weights: {weights}, bias: {bias:.3f}")
    
    print("\n2. Testing Closed Form Solution:")
    # Create fit method for closed form
    cf_method = ml_core.glm.LinearRegressionFitMethod(
        type=ml_core.glm.LinearRegressionType.CLOSED_FORM
    )
    
    lr_cf = ml_core.glm.LinearRegression(cf_method)
    lr_cf.fit(X, y)
    
    for sample in test_samples:
        prediction = lr_cf.predict(sample)
        print(f"CF - Sample {sample}: Predicted = {prediction:.3f}")
    
    weights, bias = lr_cf.get_coefficients()
    print(f"CF - Learned weights: {weights}, bias: {bias:.3f}")

if __name__ == "__main__":
    test_linear_regression()