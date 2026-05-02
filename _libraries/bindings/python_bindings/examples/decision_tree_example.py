#!/usr/bin/env python3
"""
Example usage of the Decision Tree Python bindings
"""

import numpy as np
import ml_core

def test_decision_tree():
    print("Testing Decision Tree...")
    
    # Create sample data
    # Features: [[feature1, feature2], ...]
    X = [
        [0, 0],
        [0, 1], 
        [1, 0],
        [1, 1],
        [0, 0],
        [1, 1]
    ]
    
    # Target classes
    y = [0, 1, 1, 0, 0, 0]
    
    # Create and train decision tree
    dt = ml_core.decision_tree.DecisionTree(ml_core.decision_tree.SplitCriterion.GINI)
    dt.fit(X, y, max_depth=3)
    
    # Make predictions
    test_samples = [[0, 1], [1, 0], [1, 1]]
    
    print("Predictions:")
    for i, sample in enumerate(test_samples):
        prediction = dt.predict(sample)
        print(f"Sample {sample}: Predicted class = {prediction}")

if __name__ == "__main__":
    test_decision_tree()