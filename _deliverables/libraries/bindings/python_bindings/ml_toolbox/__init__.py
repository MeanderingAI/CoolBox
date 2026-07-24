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
- kmeans: K-Means clustering with K-Means++ initialisation (backed by
          IndexedPriorityQueue + VanEmdeBoasTree from trekker::DATASTRUCTURE)
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
>>> # K-Means clustering
>>> from ml_toolbox import kmeans
>>> km = kmeans.KMeans(n_clusters=3, init=kmeans.InitMethod.KMEANSPP)
>>> km.fit(X)  # X is a 2-D numpy array (n_samples × n_features)
>>> labels = km.get_labels()
>>> centroids = km.get_centroids()
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


try:
    from ..metadata_client import Client, create_default, for_endpoint
except ImportError:
    from metadata_client import Client, create_default, for_endpoint


# Patch: Import the extension directly for local development
try:
    from . import ml_core as _ml_core
except ImportError as e:
    raise ImportError(
        "Failed to import ml_core extension. Please build the package first: python setup.py build_ext --inplace"
    ) from e
ml_core = _ml_core

try:
    from . import battery_simulator
except ImportError:
    battery_simulator = None

try:
    from . import graphics
except ImportError:
    graphics = None

__all__ = [
    "Client",
    "create_default",
    "for_endpoint",
    "deep_learning",
    "distributed",
    "decision_tree",
    "dimensionality_reduction",
    "kmeans",
    "bayesian_network",
    "hidden_markov_model",
    "generalized_linear_model",
    "support_vector_machine",
    "tracker",
    "computer_vision",
    "nlp",
    "multi_arm_bandit",
    "battery_simulator",
    "graphics",
]
