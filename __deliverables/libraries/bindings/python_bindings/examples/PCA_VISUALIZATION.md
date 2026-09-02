# PCA Visualization Example

This example demonstrates how to use the `ml_core.dimensionality_reduction.PCA` class with matplotlib for visualization.

## Quick Start

```bash
cd /home/mehran/wkspace/kf/python_bindings
./run_pca_example.sh
```

This script will:
1. Build the Python bindings
2. Install matplotlib if needed
3. Run the comprehensive PCA visualization example
4. Generate 6 PNG files with different visualizations

## Manual Setup

If you prefer to run manually:

```bash
# 1. Build the bindings
cd python_bindings
python3 setup.py build_ext --inplace

# 2. Install dependencies
pip3 install numpy matplotlib

# 3. Run the example
python3 examples/example_pca_visualization.py
```

## What the Example Shows

The example creates 6 different visualizations:

### 1. **2D Visualization** (`pca_2d_visualization.png`)
- Reduces 5D data to 2D
- Shows 3 distinct clusters before and after PCA transformation
- Demonstrates how PCA finds optimal 2D projection

### 2. **Scree Plot** (`pca_scree_plot.png`)
- Shows explained variance for each component
- Includes cumulative variance plot
- Helps determine optimal number of components

### 3. **Scaling Comparison** (`pca_scaling_comparison.png`)
- Compares PCA with and without feature scaling
- Shows why scaling matters when features have different units
- Demonstrates the effect on explained variance

### 4. **3D Projection** (`pca_3d_visualization.png`)
- Reduces 10D data to 3D
- Interactive 3D scatter plot showing cluster separation
- Includes 2D projection for comparison

### 5. **Component Loadings** (`pca_loadings.png`)
- Shows which original features contribute to each PC
- Bar charts for first 3 principal components
- Helps interpret what each component represents

### 6. **Reconstruction Error** (`pca_reconstruction_error.png`)
- Shows how reconstruction quality improves with more components
- Helps choose the right trade-off between compression and accuracy

## Simple Usage Example

```python
import numpy as np
import matplotlib.pyplot as plt
import ml_core.dimensionality_reduction as dr

# Generate sample data
X = np.random.randn(100, 10)

# Apply PCA
pca = dr.PCA(n_components=2, center=True, scale=False)
X_transformed = pca.fit_transform(X)

# Get explained variance
explained_var = pca.get_explained_variance_ratio()
print(f"Explained variance: {explained_var.sum():.2%}")

# Plot
plt.scatter(X_transformed[:, 0], X_transformed[:, 1])
plt.xlabel(f'PC1 ({explained_var[0]:.1%})')
plt.ylabel(f'PC2 ({explained_var[1]:.1%})')
plt.title('PCA Projection')
plt.show()
```

## Common Visualization Patterns

### Scree Plot

```python
# Fit PCA with all components
pca = dr.PCA(n_components=0, center=True, scale=False)
pca.fit(X)

# Get explained variance
explained_var = pca.get_explained_variance_ratio()
cumulative_var = np.cumsum(explained_var)

# Plot
plt.figure(figsize=(10, 5))
plt.subplot(1, 2, 1)
plt.bar(range(1, len(explained_var) + 1), explained_var)
plt.xlabel('Component')
plt.ylabel('Explained Variance Ratio')
plt.title('Scree Plot')

plt.subplot(1, 2, 2)
plt.plot(range(1, len(cumulative_var) + 1), cumulative_var, 'o-')
plt.axhline(y=0.95, color='r', linestyle='--', label='95%')
plt.xlabel('Number of Components')
plt.ylabel('Cumulative Variance')
plt.legend()
plt.show()
```

### 3D Scatter Plot

```python
from mpl_toolkits.mplot3d import Axes3D

# Reduce to 3D
pca = dr.PCA(n_components=3, center=True, scale=False)
X_3d = pca.fit_transform(X)

# Plot
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')
ax.scatter(X_3d[:, 0], X_3d[:, 1], X_3d[:, 2], alpha=0.6)
ax.set_xlabel('PC1')
ax.set_ylabel('PC2')
ax.set_zlabel('PC3')
plt.show()
```

### Component Loadings Heatmap

```python
import matplotlib.pyplot as plt

# Fit PCA
pca = dr.PCA(n_components=3, center=True, scale=False)
pca.fit(X)

# Get components (loadings)
components = pca.get_components()

# Plot heatmap
plt.figure(figsize=(10, 6))
plt.imshow(components, aspect='auto', cmap='coolwarm')
plt.colorbar(label='Loading')
plt.xlabel('Principal Component')
plt.ylabel('Feature')
plt.title('PCA Component Loadings')
plt.show()
```

## Tips for Visualization

1. **Always label axes with explained variance**:
   ```python
   plt.xlabel(f'PC1 ({explained_var[0]:.1%} variance)')
   ```

2. **Use scree plots to choose number of components**:
   - Look for "elbow" in the curve
   - Or use 90-95% cumulative variance threshold

3. **Scale your data when features have different units**:
   ```python
   pca = dr.PCA(n_components=2, center=True, scale=True)
   ```

4. **Color by cluster/category for better insights**:
   ```python
   for label in unique_labels:
       mask = y == label
       plt.scatter(X_pca[mask, 0], X_pca[mask, 1], label=f'Class {label}')
   plt.legend()
   ```

5. **Check reconstruction error to validate component choice**:
   ```python
   X_reconstructed = pca.inverse_transform(X_transformed)
   error = np.linalg.norm(X - X_reconstructed) / np.linalg.norm(X)
   print(f"Relative error: {error:.4f}")
   ```

## Requirements

- Python 3.6+
- numpy
- matplotlib
- ml_core (built from this project)

## Troubleshooting

**Import error for ml_core**:
```bash
# Make sure you're in the python_bindings directory
cd python_bindings
python3 setup.py build_ext --inplace
export PYTHONPATH=$PYTHONPATH:$(pwd)
```

**matplotlib not found**:
```bash
pip3 install matplotlib
```

**Build errors**:
Make sure you have the required C++ libraries:
- Eigen3: `sudo apt install libeigen3-dev`
- pybind11: `pip3 install pybind11`
