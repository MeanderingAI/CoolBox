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

# Dependencies

## GSL

This library uses libgsl, on ubuntu it can be installed with

```
sudo apt install libgsl-dev
```

## Eigen

This library is automatically installed by cmake during compilation

## GoogleTest

This library is automatically installed by cmake during compilation

# Compiling

use the `./clean&build` script.

Generated libraries can be found in;

`build/src/lib*.so`

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
