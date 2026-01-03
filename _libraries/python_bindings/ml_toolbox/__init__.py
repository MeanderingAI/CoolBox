"""
ML ToolBox - Comprehensive Machine Learning Library
====================================================

A high-performance C++ machine learning library with Python bindings.

Main Modules
------------
- deep_learning: Neural networks, CNNs, RNNs, Transformers, LLMs
- distributed: Distributed training (data parallel, model parallel, federated)
- decision_tree: Decision trees, random forests, boosting
- dimensionality_reduction: PCA, SVD, UMAP, KNN
- bayesian_network: Bayesian networks and inference
- hidden_markov_model: Hidden Markov Models
- generalized_linear_model: Linear/logistic regression, GLMs
- support_vector_machine: SVM classification and regression
- tracker: Kalman filters, particle filters
- computer_vision: Image processing and CV algorithms
- nlp: Natural language processing
- multi_arm_bandit: Multi-armed bandit algorithms

Quick Start
-----------
>>> import ml_toolbox as ml
>>> from ml_toolbox import deep_learning as dl
>>> 
>>> # Create a neural network
>>> model = dl.binary_classifier(input_dim=10)
>>> 
>>> # Or create an LLM
>>> llm = dl.language_model(vocab_size=10000, context_length=512)
>>> 
>>> # Distributed training
>>> from ml_toolbox import distributed
>>> trainer = distributed.DataParallelTrainer(num_workers=4)

Examples
--------
See the examples/ directory for comprehensive usage examples:
- llm_example.py: Large Language Models
- distributed_training_demo.py: Distributed training
- templates_demo.py: Neural network templates
"""

__version__ = "0.2.0"

# Import the C++ extension
try:
    from .ml_core import *
except ImportError:
    # Try to import from the parent directory (during development)
    try:
        import ml_core
        # Re-export all symbols
        import sys
        _module = sys.modules[__name__]
        for attr in dir(ml_core):
            if not attr.startswith('_'):
                setattr(_module, attr, getattr(ml_core, attr))
    except ImportError as e:
        raise ImportError(
            "Failed to import ml_core extension. "
            "Please build the package first: python setup.py build_ext --inplace"
        ) from e

__all__ = [
    "deep_learning",
    "distributed", 
    "decision_tree",
    "dimensionality_reduction",
    "bayesian_network",
    "hidden_markov_model",
    "generalized_linear_model",
    "support_vector_machine",
    "tracker",
    "computer_vision",
    "nlp",
    "multi_arm_bandit",
]
