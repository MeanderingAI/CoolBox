# Compiling

## Makefile Commands

- `make build` — Build all C++ libraries
- `make build-apps` — Build all C++ apps
- `make build-services` — Build all C++ services
- `make build-service-manager` — Build the service_manager app
- `make build-emscripten` — Build Emscripten JS/WASM bindings (set EMS_MODULARIZE=0 for synchronous JS, default is async)
- `make clean` — Remove all build artifacts
- `make list-apps` — List all built apps
- `make list-libs` — List all built libraries
- `make list-services` — List all built services
- `make help-run` — Show runnable apps and demos

### Example: Build Emscripten bindings with MODULARIZE=0 (sync JS)

```sh
make build-emscripten EMS_MODULARIZE=OFF
```

### Example: Build Emscripten bindings with MODULARIZE=1 (async JS, default)

```sh
make build-emscripten
```

# Tool Box

The cmake generates shared objects which can be used with other projects for each of these categories of machine learning.
We also include gunit tests for the entire suite.

## Filters

Here we provide a Kalman Filter library which contains CPP implementation of the standard array of implementations;

1. Standard Kalman Filter
2. Extended Kalman Filter
3. Unscented Kalman Filter
4. Sequential Monte Carlo


## Generalized Linear Models

Current implementation includes Linear Regression with;

1. Stochastic Gradient Decent
2. Closed Form Solution

## Decision Trees

Here we provide the following Decision Tree Estimators;

1. Entropy 
2. Gini Index

With the following variations;

1. Rule Set
2. Random Forest
3. Boosted Trees

## Statistical Distributions

Here we provide the following Standard Statistical Distributions;

1. Bernoulli Distribution
2. Binomial Distribution
3. Categorical Distribution
4. Exponential Distribution
5. Gamma Distribution
6. Inverse Gaussian Distribution
7. Laplace Distribution
8. Multinomial Distribution
9. Normal Distribution
10. Poisson Distribution

These distributions have implementations of pdf, log_pdf, cdf, log_cdf, and sampling.

## Hidden Markov Model

Traditional implementation of forward and backward algorithm.

## Multi-Arm Bandit

Here we provide the following multi-arm bandit implementations;

1. Epsilon Greedy Agent
2. Decaying Epsilon Greedy Agent
3. Upper Confidence Bound Agent 
4. Thompson Sampling Agent

## Support Vector Machine

Here we provide implementation of support vector machines with the following kernels;

1. Linear
2. Polynomial
3. Gaussian 
4. Sigmoid

## Latent Sentiment Analysis

Perform latent sentiment analysis on text using matrix factorization:

1. Document-term matrix factorization
2. Latent feature extraction for documents and terms
3. SGD optimization with regularization
4. Sentiment prediction and reconstruction

## Hidden Markov Models

Provides comprehensive HMM implementation:

1. Forward-Backward algorithm (evaluation)
2. Viterbi algorithm (decoding)
3. Baum-Welch algorithm (training)
4. Log-likelihood computation
5. Multiple observation sequences support

## Bayesian Networks

Directed Acyclic Graph (DAG) probabilistic models:

1. Node and edge management
2. Conditional Probability Tables (CPT)
3. Joint probability calculation
4. Probabilistic inference
5. Evidence-based reasoning

## Marked Point Process

Temporal event modeling with associated marks (labels):

1. Self-exciting Hawkes processes
2. Multi-mark event sequences
3. Conditional intensity prediction
4. Event sequence generation
5. Maximum likelihood estimation
6. Applications: financial transactions, user activity logs, earthquakes

## Piecewise Conditional Intensity Models (PCIM)

Non-stationary temporal point process modeling:

1. Multiple intensity function types:
   - Constant intensity
   - Linear intensity
   - Exponential decay
   - Hawkes self-exciting
   - Cox proportional hazards
2. Uniform and adaptive interval creation
3. Regime change detection
4. Model selection (AIC/BIC)
5. Applications: market microstructure, crime patterns, healthcare monitoring

## Dimensionality Reduction

Comprehensive suite of dimensionality reduction algorithms for feature extraction, visualization, and manifold learning:

### 1. **Singular Value Decomposition (SVD)**
   - Thin and full SVD computation using Eigen's BDCSVD
   - Low-rank matrix approximation and reconstruction
   - Matrix rank estimation with tolerance
   - Explained variance analysis
   - Applications: matrix compression, noise reduction, latent semantic analysis

### 2. **Principal Component Analysis (PCA)**
   - Linear dimensionality reduction via SVD
   - Data centering and optional standardization (scaling)
   - Principal component extraction (loadings)
   - Variance explained computation
   - Forward and inverse transformation
   - Applications: data visualization, feature extraction, preprocessing

### 3. **k-Nearest Neighbors (KNN)**
   - Efficient brute-force nearest neighbor search
   - Multiple distance metrics: Euclidean, Manhattan, Cosine
   - Pairwise distance computation
   - Self-exclusion option for training data queries
   - Foundation for manifold learning algorithms

### 4. **Uniform Manifold Approximation and Projection (UMAP)**
   - State-of-the-art non-linear dimensionality reduction
   - Preserves both local and global data structure
   - Fuzzy simplicial set construction
   - Stochastic gradient descent optimization
   - Superior to t-SNE for many applications
   - Applications: visualization, cluster analysis, anomaly detection

**Example (C++ with Eigen):**

```cpp
#include "dimensionality_reduction/pca.h"
#include "dimensionality_reduction/knn.h"
#include "dimensionality_reduction/umap.h"

// PCA for linear dimensionality reduction
Eigen::MatrixXd X = /* your data (n_samples x n_features) */;
PCA pca(10, true, false);  // 10 components, center=true, scale=false
pca.fit(X);
Eigen::MatrixXd X_pca = pca.transform(X);

// KNN for nearest neighbor queries
KNN knn(15, "euclidean");  // k=15 neighbors
knn.fit(X);
auto [indices, distances] = knn.kneighbors();

// UMAP for non-linear manifold learning
UMAP umap(2, 15, 0.1, "euclidean", 1.0, 200, 42);
// n_components=2, n_neighbors=15, min_dist=0.1
Eigen::MatrixXd X_umap = umap.fit_transform(X);
```

**Key Differences:**
- **PCA**: Linear, fast, preserves global variance structure
- **UMAP**: Non-linear, slower, preserves local neighborhood structure and global topology
- **t-SNE vs UMAP**: UMAP scales better, faster convergence, preserves more global structure

**Notes:**
- Use PCA when relationships are linear or for quick exploration
- Use UMAP for complex manifolds and better cluster separation
- For very high-dimensional data (>50 features), consider PCA preprocessing before UMAP
- All algorithms exposed via Python bindings in `ml_core.dimensionality_reduction`
- Scaling recommended when features have different units/scales

# Dependencies

## CMAKE

OSX
```
brew install cmake
```

## GSL

This library uses libgsl for the distribution generation, on ubuntu it can be installed with

Ubuntu
```
sudo apt install libgsl-dev
```

OSX
```
brew install gsl
```

## Bison

```
brew install bison
```

## Doxygen

```
brew install doxygen
```

## Eigen

This library is automatically installed by cmake during compilation

## GoogleTest

This library is automatically installed by cmake during compilation

# Compiling


## Dependencies

- C++17 compiler (clang++/g++)
- CMake 3.12+
- Python 3 (for bindings)
- **Rust (cargo)**: Required for HTTP/3/QUIC support (quiche library)
   - Install on macOS: `brew install rust`
   - Or: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`

Use the `./clean&build` script to build everything.

Generated libraries can be found in;

`build/libraries/src/lib*.so`

# Python Bindings

Python bindings are available for all machine learning algorithms via pybind11. See the `python_bindings/` directory for detailed documentation and examples.

## Quick Start

```bash
cd python_bindings
./build.sh
python3 test_bindings.py
```

## Available Modules

- `ml_core.decision_tree` - Decision tree algorithms
- `ml_core.svm` - Support Vector Machines with various kernels
- `ml_core.bayesian_network` - Bayesian Network inference
- `ml_core.hmm` - Hidden Markov Models
- `ml_core.glm` - Generalized Linear Models
- `ml_core.multi_arm_bandit` - Multi-arm bandit algorithms
- `ml_core.marked_point_process` - Marked point processes and PCIM
- `ml_core.latent_sentiment_analysis` - Latent sentiment analysis
- `ml_core.tracker` - Kalman filters

See `python_bindings/README.md` and `python_bindings/examples/` for comprehensive usage examples.
