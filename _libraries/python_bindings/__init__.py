# ML Core Python Package
"""
ML Core - Comprehensive Machine Learning Library

This package provides Python bindings for a comprehensive machine learning
library implemented in C++. It includes algorithms for:

- Decision Trees and Random Forests
- Support Vector Machines with multiple kernels
- Bayesian Networks with probabilistic inference
- Hidden Markov Models with Baum-Welch training
- Linear Regression with multiple solvers
- Multi-arm Bandit algorithms
- Kalman Filters for state estimation

Example usage:
    import ml_core
    
    # Decision Tree
    dt = ml_core.decision_tree.DecisionTree()
    dt.fit(X, y, max_depth=5)
    predictions = [dt.predict(sample) for sample in test_data]
    
    # Support Vector Machine
    kernel = ml_core.svm.RBFKernel(gamma=0.1)
    svm = ml_core.svm.SVM(kernel)
    svm.fit(X, y)
    
    # Hidden Markov Model
    hmm = ml_core.hmm.HMM(states=3, observations=4)
    hmm.train(observation_sequences)
"""

__version__ = "0.1.0"
__author__ = "ML Core Team"

# Note: The actual ml_core module is a compiled C++ extension
# It will be available after building the bindings

try:
    # Try to import the compiled extension
    from . import ml_core
    __all__ = ['ml_core']
except ImportError:
    # Extension not built yet
    import warnings
    warnings.warn(
        "ML Core C++ extension not found. "
        "Please build the bindings first using 'python setup.py build_ext --inplace' "
        "or 'pip install .'",
        ImportWarning
    )
    __all__ = []