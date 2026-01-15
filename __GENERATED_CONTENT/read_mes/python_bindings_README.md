# ML Core Python Bindings

This directory contains Python bindings for the ML Core C++ library, providing access to various machine learning algorithms including Decision Trees, Support Vector Machines, Bayesian Networks, Hidden Markov Models, Linear Regression, and Multi-arm Bandits.

## Features

### Algorithms Included

1. **Decision Trees**
   - Support for GINI and Entropy splitting criteria
   - Configurable maximum depth
   - Integer feature support

2. **Support Vector Machines**
   - Multiple kernel types: Linear, RBF, Polynomial, Sigmoid
   - Eigen matrix/vector support for efficient computation
   - Binary classification

3. **Bayesian Networks**
   - Directed Acyclic Graph (DAG) structure
   - Conditional Probability Tables (CPT)
   - Probabilistic inference
   - Joint probability calculation

4. **Hidden Markov Models**
   - Forward-Backward algorithm
   - Viterbi decoding (most likely state sequence)
   - Baum-Welch training
   - Log-likelihood calculation

5. **Linear Regression**
   - Gradient descent and closed-form solutions
   - Configurable learning parameters
   - Weight and bias extraction

6. **Multi-arm Bandits**
   - Bandit arm simulation
   - Reward probability estimation
   - Pull count tracking

7. **Marked Point Process**
   - Temporal event modeling with marks (labels)
   - Self-exciting Hawkes processes
   - Event intensity prediction
   - Sequence generation
   - Applications: financial transactions, user activity, earthquakes

8. **Piecewise Conditional Intensity Models (PCIM)**
   - Non-stationary temporal point processes
   - Multiple intensity function types (Constant, Linear, Exponential, Hawkes, Cox)
   - Adaptive interval creation based on event density
   - Regime change detection
   - Applications: market microstructure, crime patterns, healthcare monitoring

9. **Latent Sentiment Analysis**
   - Matrix factorization for sentiment modeling
   - Document-term latent features
   - Collaborative filtering approach
   - SGD optimization with regularization

10. **Singular Value Decomposition (SVD) & Principal Component Analysis (PCA)**
    - Native C++ implementations using Eigen for high performance
    - Full SVD decomposition with thin/full matrix options
    - PCA with centering and optional scaling
    - Explained variance computation and low-rank approximation
    - Dimensionality reduction for visualization and feature extraction

11. **k-Nearest Neighbors (KNN)**
    - Efficient nearest neighbor search
    - Multiple distance metrics (Euclidean, Manhattan, Cosine)
    - Pairwise distance computation
    - Foundation for manifold learning algorithms

12. **Uniform Manifold Approximation and Projection (UMAP)**
    - State-of-the-art non-linear dimensionality reduction
    - Preserves both local and global data structure
    - Superior to t-SNE for many applications
    - Configurable parameters for fine-tuning embeddings
    - Fast convergence with stochastic gradient descent## Installation

### Prerequisites

- Python 3.6 or higher
- CMake 3.12 or higher
- C++17 compatible compiler
- Eigen3 library
- pybind11

### Build from Source

1. **Install pybind11:**
   ```bash
   pip install pybind11[global]
   ```

2. **Install numpy:**
   ```bash
   pip install numpy
   ```

3. **Build using setup.py:**
   ```bash
   cd python_bindings
   python setup.py build_ext --inplace
   ```

4. **Or build using CMake:**
   ```bash
   cd python_bindings
   mkdir build
   cd build
   cmake ..
   make
   ```

### Install as Package

```bash
cd python_bindings
pip install .
```

## Usage Examples

### Decision Tree

```python
import ml_core

# Create sample data
X = [[0, 0], [0, 1], [1, 0], [1, 1]]
y = [0, 1, 1, 0]

# Create and train decision tree
dt = ml_core.decision_tree.DecisionTree(ml_core.decision_tree.SplitCriterion.GINI)
dt.fit(X, y, max_depth=3)

# Make prediction
prediction = dt.predict([1, 0])
print(f"Predicted class: {prediction}")
```

### Support Vector Machine

```python
import numpy as np
import ml_core

# Create data
X = np.array([[1.0, 2.0], [2.0, 3.0], [6.0, 5.0], [7.0, 7.0]])
y = np.array([-1.0, -1.0, 1.0, 1.0])

# Create SVM with RBF kernel
kernel = ml_core.svm.RBFKernel(gamma=0.5)
svm = ml_core.svm.SVM(kernel)
svm.fit(X, y)

# Predict
prediction = svm.predict(np.array([3.0, 4.0]))
print(f"Predicted: {prediction}")
```

### Hidden Markov Model

```python
import numpy as np
import ml_core

# Create HMM
hmm = ml_core.hmm.HMM(states=2, observations=3)

# Set parameters
initial_probs = np.array([0.6, 0.4])
hmm.set_initial_probabilities(initial_probs)

transition_matrix = np.array([[0.7, 0.3], [0.4, 0.6]])
hmm.set_transition_matrix(transition_matrix)

emission_matrix = np.array([[0.5, 0.4, 0.1], [0.1, 0.3, 0.6]])
hmm.set_emission_matrix(emission_matrix)

# Decode sequence
observations = [0, 1, 2, 1, 0]
states = hmm.get_most_likely_states(observations)
print(f"Most likely states: {states}")
```

### Linear Regression

```python
import ml_core

# Create data
X = [[1.0, 2.0], [2.0, 3.0], [3.0, 4.0]]
y = [7.0, 9.5, 12.0]

# Create fit method
method = ml_core.glm.LinearRegressionFitMethod(
    num_iterations=1000,
    learning_rate=0.01,
    type=ml_core.glm.LinearRegressionType.GRADIENT_DESCENT
)

# Train model
lr = ml_core.glm.LinearRegression(method)
lr.fit(X, y)

# Predict
prediction = lr.predict([4.0, 5.0])
print(f"Predicted: {prediction}")

# Get coefficients
weights, bias = lr.get_coefficients()
print(f"Weights: {weights}, Bias: {bias}")
```

### Bayesian Network

```python
import numpy as np
import ml_core

# Create network
bn = ml_core.bayesian_network.BayesianNetwork()

# Add nodes
weather = bn.add_node("Weather", ["Sunny", "Rainy"])
grass = bn.add_node("Grass", ["Wet", "Dry"])

# Add edge
bn.add_edge(weather, grass)

# Set CPTs
weather_cpt = np.array([[0.7, 0.3]])
bn.set_cpt(weather, weather_cpt)

grass_cpt = np.array([[0.2, 0.8], [0.9, 0.1]])
bn.set_cpt(grass, grass_cpt)

# Inference
evidence = {weather: 0}  # Weather=Sunny
prob = bn.infer(grass, 0, evidence)  # P(Grass=Wet | Weather=Sunny)
print(f"P(Grass=Wet | Weather=Sunny) = {prob}")
```

### Marked Point Process

```python
import ml_core

# Create marked point process with 2 mark types
mpp = ml_core.marked_point_process.MarkedPointProcess(
    num_marks=2,
    learning_rate=0.01,
    max_iterations=500
)

# Training data: sequences of (times, marks)
event_times = [
    [0.5, 1.2, 2.3, 3.1],  # Sequence 1
    [0.8, 1.5, 2.7, 3.9]   # Sequence 2
]
event_marks = [
    [0, 1, 0, 1],  # Mark types for sequence 1
    [1, 0, 1, 0]   # Mark types for sequence 2
]

# Train the model
mpp.fit(event_times, event_marks)

# Predict intensity at time t=2.0 given history
history_times = [0.5, 1.2]
history_marks = [0, 1]
intensities = mpp.predict_intensity(2.0, history_times, history_marks)
print(f"Intensities for each mark: {intensities}")

# Generate synthetic sequence
new_times, new_marks = mpp.generate_sequence(time_horizon=5.0, max_events=20)
print(f"Generated {len(new_times)} events")

# Get learned parameters
mu = mpp.get_base_intensity()
alpha = mpp.get_excitation_matrix()
beta = mpp.get_decay_rate()
print(f"Base intensity: {mu}")
print(f"Excitation matrix:\n{alpha}")
print(f"Decay rate: {beta}")
```

### Piecewise Conditional Intensity Model

```python
import ml_core

# Create PCIM with 5 time intervals
pcim = ml_core.marked_point_process.PiecewiseConditionalIntensityModel(
    num_intervals=5,
    learning_rate=0.01,
    max_iterations=500
)

# Event sequences (just times, no marks for PCIM)
event_sequences = [
    [0.1, 0.5, 1.2, 2.3, 3.1, 4.5],
    [0.3, 0.8, 1.5, 2.7, 3.9, 4.2]
]

# Option 1: Create uniform intervals
pcim.create_uniform_intervals(
    time_min=0.0,
    time_max=5.0,
    intensity_type=ml_core.marked_point_process.IntensityType.HAWKES
)

# Option 2: Create adaptive intervals based on data
all_events = [t for seq in event_sequences for t in seq]
pcim.create_adaptive_intervals(
    all_events,
    intensity_type=ml_core.marked_point_process.IntensityType.HAWKES
)

# Train the model
pcim.fit(event_sequences)

# Predict intensity at a specific time
history = [0.1, 0.5, 1.2]
intensity = pcim.predict_intensity(2.0, history)
print(f"Predicted intensity at t=2.0: {intensity}")

# Generate synthetic sequence
generated = pcim.generate_sequence(time_horizon=5.0, max_events=50)
print(f"Generated {len(generated)} events")

# Get model parameters for each interval
intervals = pcim.get_intervals()
for i, interval in enumerate(intervals):
    params = pcim.get_interval_parameters(i)
    print(f"Interval {i} [{interval.start_time:.2f}, {interval.end_time:.2f}]: {params}")

# Model selection
aic, bic = pcim.compute_information_criteria(event_sequences)
print(f"AIC: {aic:.2f}, BIC: {bic:.2f}")
```

### Latent Sentiment Analysis

```python
import numpy as np
import ml_core

# Document-term matrix (rows: documents, cols: terms)
# Values can be counts, TF-IDF, etc.
X = np.array([
    [2, 0, 1, 1, 1],  # Document 1
    [0, 2, 1, 1, 0],  # Document 2
    [0, 1, 1, 0, 2],  # Document 3
    [1, 0, 1, 2, 1]   # Document 4
])

# Create and train model
lsa = ml_core.latent_sentiment_analysis.LatentSentimentAnalysis(
    latent_features=2,
    learning_rate=0.05,
    lambda_reg=0.01,
    max_iterations=500
)

lsa.train(X)

# Get learned latent factors
doc_features = lsa.get_document_features()
term_features = lsa.get_term_features()

print(f"Document features shape: {doc_features.shape}")
print(f"Term features shape: {term_features.shape}")

# Predict score for document-term pair
score = lsa.predict_score(doc_index=0, term_index=2)
print(f"Predicted score for doc 0, term 2: {score:.3f}")
```

### SVD (Singular Value Decomposition)

```python
import numpy as np
import ml_core.dimensionality_reduction as dr

# Create a matrix
X = np.array([
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9],
    [10, 11, 12]
], dtype=np.float64)

# Compute SVD
svd = dr.SVD(compute_full_matrices=False)
svd.compute(X)

# Get components
U = svd.get_U()  # Left singular vectors
S = svd.get_singular_values()  # Singular values
V = svd.get_V()  # Right singular vectors

# Reconstruct original matrix
X_reconstructed = svd.reconstruct()

# Low-rank approximation
X_approx = svd.reconstruct(num_components=2)

# Get explained variance ratio
explained_var = svd.explained_variance_ratio()
print(f"Explained variance: {explained_var}")

# Estimate rank
rank = svd.rank()
print(f"Matrix rank: {rank}")
```

### PCA (Principal Component Analysis)

```python
import numpy as np
import ml_core.dimensionality_reduction as dr

# Generate sample data
np.random.seed(42)
X = np.random.randn(100, 5)  # 100 samples, 5 features

# Create PCA with 3 components
pca = dr.PCA(n_components=3, center=True, scale=False)

# Fit and transform
X_transformed = pca.fit_transform(X)
print(f"Transformed shape: {X_transformed.shape}")  # (100, 3)

# Get principal components
components = pca.get_components()
print(f"Components shape: {components.shape}")  # (5, 3)

# Get explained variance
explained_var = pca.get_explained_variance_ratio()
print(f"Explained variance: {explained_var}")
print(f"Total explained: {explained_var.sum():.2%}")

# Transform new data
X_new = np.random.randn(10, 5)
X_new_transformed = pca.transform(X_new)

# Inverse transform (reconstruct)
X_reconstructed = pca.inverse_transform(X_transformed)

# Get learned statistics
mean = pca.get_mean()
scale = pca.get_scale()
```

### PCA with Scaling

```python
import ml_core.dimensionality_reduction as dr

# PCA with standardization (recommended for features with different scales)
pca_scaled = dr.PCA(n_components=3, center=True, scale=True)
pca_scaled.fit(X)

# This standardizes each feature to zero mean and unit variance
# Equivalent to sklearn's StandardScaler + PCA
```

### KNN (k-Nearest Neighbors)

```python
import numpy as np
import ml_core.dimensionality_reduction as dr

# Create sample data
X = np.random.randn(100, 5)

# Create KNN with k=5 neighbors
knn = dr.KNN(k=5, metric="euclidean")
knn.fit(X)

# Find neighbors for training data (excludes self)
indices, distances = knn.kneighbors()
print(f"Neighbor indices shape: {indices.shape}")  # (100, 5)
print(f"Neighbor distances shape: {distances.shape}")  # (100, 5)

# Find neighbors for new query points
X_query = np.random.randn(10, 5)
query_indices, query_distances = knn.kneighbors(X_query)
print(f"Query neighbors shape: {query_indices.shape}")  # (10, 5)

# Compute pairwise distances
X1 = np.random.randn(10, 5)
X2 = np.random.randn(20, 5)
dist_matrix = knn.pairwise_distances(X1, X2)
print(f"Distance matrix shape: {dist_matrix.shape}")  # (10, 20)

# Different distance metrics
knn_manhattan = dr.KNN(k=5, metric="manhattan")
knn_cosine = dr.KNN(k=5, metric="cosine")
```

### UMAP (Uniform Manifold Approximation and Projection)

```python
import numpy as np
import matplotlib.pyplot as plt
import ml_core.dimensionality_reduction as dr

# Generate high-dimensional data (e.g., 100 samples, 50 features)
np.random.seed(42)
X = np.random.randn(100, 50)

# Create UMAP with default parameters
umap = dr.UMAP(
    n_components=2,      # Reduce to 2D
    n_neighbors=15,      # Number of neighbors to consider
    min_dist=0.1,        # Minimum distance between points in embedding
    metric="euclidean",  # Distance metric
    learning_rate=1.0,   # Learning rate for optimization
    n_epochs=200,        # Number of training epochs
    random_state=42      # For reproducibility
)

# Fit and transform data
X_umap = umap.fit_transform(X)
print(f"UMAP embedding shape: {X_umap.shape}")  # (100, 2)

# Get the learned embedding
embedding = umap.get_embedding()

# Plot UMAP results
plt.figure(figsize=(10, 8))
plt.scatter(X_umap[:, 0], X_umap[:, 1], alpha=0.6)
plt.xlabel('UMAP 1')
plt.ylabel('UMAP 2')
plt.title('UMAP Projection')
plt.show()

# UMAP with different parameters
umap_3d = dr.UMAP(
    n_components=3,      # 3D embedding
    n_neighbors=30,      # More neighbors = preserve more global structure
    min_dist=0.0,        # Tighter clustering
    n_epochs=500         # More epochs = better optimization
)
X_umap_3d = umap_3d.fit_transform(X)

# Quick 2D projection for visualization
umap_viz = dr.UMAP(n_components=2, n_neighbors=15, min_dist=0.1)
X_2d = umap_viz.fit_transform(X)
```

### Comparing PCA vs UMAP

```python
import numpy as np
import matplotlib.pyplot as plt
import ml_core.dimensionality_reduction as dr

# Generate data with non-linear structure
from sklearn.datasets import make_swiss_roll
X, color = make_swiss_roll(n_samples=1000, noise=0.1)

# PCA (linear)
pca = dr.PCA(n_components=2)
X_pca = pca.fit_transform(X)

# UMAP (non-linear)
umap = dr.UMAP(n_components=2, n_neighbors=30)
X_umap = umap.fit_transform(X)

# Plot comparison
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

ax1.scatter(X_pca[:, 0], X_pca[:, 1], c=color, cmap='viridis')
ax1.set_title('PCA (Linear)')
ax1.set_xlabel('PC1')
ax1.set_ylabel('PC2')

ax2.scatter(X_umap[:, 0], X_umap[:, 1], c=color, cmap='viridis')
ax2.set_title('UMAP (Non-linear)')
ax2.set_xlabel('UMAP 1')
ax2.set_ylabel('UMAP 2')

plt.tight_layout()
plt.show()
```

### SVD / PCA (C++ / Eigen guidance)

If you need to perform SVD or PCA in native code (for speed or integration with other C++ components), Eigen provides efficient SVD implementations. Below is a short C++ pattern you can reuse inside the C++ codebase before exposing results to Python:

```cpp
// Center the data matrix (rows = samples, columns = features)
Eigen::MatrixXd Xc = X.rowwise() - X.colwise().mean().transpose();

// Compute thin SVD
Eigen::BDCSVD<Eigen::MatrixXd> svd(Xc, Eigen::ComputeThinU | Eigen::ComputeThinV);
Eigen::MatrixXd U = svd.matrixU();
Eigen::VectorXd S = svd.singularValues();
Eigen::MatrixXd V = svd.matrixV();

// Project to k components: scores = Xc * V.leftCols(k)
```

In Python bindings, you can now directly use the `ml_core.dimensionality_reduction.SVD` and `ml_core.dimensionality_reduction.PCA` classes which wrap these operations.


## Module Structure

- `ml_core.decision_tree`: Decision tree algorithms
- `ml_core.svm`: Support Vector Machine with various kernels
- `ml_core.bayesian_network`: Bayesian Network inference
- `ml_core.hmm`: Hidden Markov Model algorithms
- `ml_core.glm`: Generalized Linear Models (Linear Regression)
- `ml_core.multi_arm_bandit`: Multi-arm bandit algorithms
- `ml_core.marked_point_process`: Marked point processes and PCIM
- `ml_core.latent_sentiment_analysis`: Latent sentiment analysis
- `ml_core.dimensionality_reduction`: SVD and PCA for dimensionality reduction
- `ml_core.tracker`: Kalman filters and state estimation (if available)

## API Documentation

### Data Types

The bindings support various data types:
- Python lists of integers/floats
- NumPy arrays (automatically converted to Eigen matrices/vectors)
- Standard Python data structures (maps, vectors)

### Error Handling

The bindings preserve C++ exceptions and convert them to appropriate Python exceptions.

### Memory Management

All memory management is handled automatically by pybind11. No manual cleanup is required.

## Building Integration

### CMake Integration

To integrate with the main project's CMake:

```cmake
add_subdirectory(python_bindings)
```

### Dependencies

The bindings require:
- Eigen3 (for matrix operations)
- pybind11 (for Python-C++ binding)
- The ML Core C++ source files

## Troubleshooting

### Common Issues

1. **Import Error**: Ensure the module is built and in Python path
2. **Compilation Error**: Check that all dependencies are installed
3. **Runtime Error**: Verify data types match expected inputs

### Build Issues

1. **Eigen not found**: Install libeigen3-dev or specify EIGEN3_INCLUDE_DIR
2. **pybind11 not found**: Install via pip or conda
3. **C++17 support**: Ensure compiler supports C++17 standard

## Examples

See the `examples/` directory for comprehensive usage examples of all algorithms.

## Contributing

When adding new algorithms to the C++ codebase:

1. Add the corresponding Python bindings in `py_ml_core.cpp`
2. Update the module documentation
3. Add usage examples
4. Test the bindings

## License

This project follows the same license as the main ML Core library.