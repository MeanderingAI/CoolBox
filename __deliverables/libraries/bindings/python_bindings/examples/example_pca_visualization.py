"""
PCA Visualization Example with matplotlib

This example demonstrates:
1. Generating synthetic data with known structure
2. Applying PCA for dimensionality reduction
3. Visualizing the results using matplotlib
4. Comparing scaled vs unscaled PCA
5. Creating scree plots and explained variance visualizations
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import ml_core.dimensionality_reduction as dr

# Set random seed for reproducibility
np.random.seed(42)

print("=" * 70)
print("PCA Visualization with matplotlib")
print("=" * 70)

# ============================================================================
# Example 1: 2D Visualization of High-Dimensional Data
# ============================================================================
print("\n1. Reducing 5D data to 2D for visualization")
print("-" * 70)

# Generate 5D data with 3 distinct clusters
n_samples_per_cluster = 50
n_features = 5

# Cluster 1: centered at [1, 1, 1, 1, 1]
cluster1 = np.random.randn(n_samples_per_cluster, n_features) * 0.5 + 1

# Cluster 2: centered at [-1, -1, -1, -1, -1]
cluster2 = np.random.randn(n_samples_per_cluster, n_features) * 0.5 - 1

# Cluster 3: centered at [2, -2, 1, -1, 0]
cluster3 = np.random.randn(n_samples_per_cluster, n_features) * 0.5 + [2, -2, 1, -1, 0]

# Combine clusters
X = np.vstack([cluster1, cluster2, cluster3])
labels = np.array([0] * n_samples_per_cluster + 
                  [1] * n_samples_per_cluster + 
                  [2] * n_samples_per_cluster)

print(f"Original data shape: {X.shape}")
print(f"Number of samples: {X.shape[0]}")
print(f"Number of features: {X.shape[1]}")

# Apply PCA to reduce to 2D
pca = dr.PCA(n_components=2, center=True, scale=False)
X_pca = pca.fit_transform(X)

print(f"\nPCA-transformed data shape: {X_pca.shape}")

# Get explained variance
explained_var = pca.get_explained_variance_ratio()
print(f"\nExplained variance by PC1: {explained_var[0]:.2%}")
print(f"Explained variance by PC2: {explained_var[1]:.2%}")
print(f"Total explained variance: {explained_var.sum():.2%}")

# Plot
plt.figure(figsize=(12, 5))

# Original data (plot first two dimensions only)
plt.subplot(1, 2, 1)
colors = ['red', 'blue', 'green']
for i in range(3):
    mask = labels == i
    plt.scatter(X[mask, 0], X[mask, 1], c=colors[i], label=f'Cluster {i+1}', alpha=0.6)
plt.xlabel('Feature 1')
plt.ylabel('Feature 2')
plt.title('Original Data (First 2 Dimensions)')
plt.legend()
plt.grid(True, alpha=0.3)

# PCA-transformed data
plt.subplot(1, 2, 2)
for i in range(3):
    mask = labels == i
    plt.scatter(X_pca[mask, 0], X_pca[mask, 1], c=colors[i], label=f'Cluster {i+1}', alpha=0.6)
plt.xlabel(f'PC1 ({explained_var[0]:.1%} variance)')
plt.ylabel(f'PC2 ({explained_var[1]:.1%} variance)')
plt.title('PCA-Transformed Data')
plt.legend()
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('pca_2d_visualization.png', dpi=150, bbox_inches='tight')
print("\n✓ Saved: pca_2d_visualization.png")
plt.show()

# ============================================================================
# Example 2: Scree Plot - Choosing Number of Components
# ============================================================================
print("\n2. Scree Plot - Determining Optimal Number of Components")
print("-" * 70)

# Generate higher-dimensional data
n_samples = 200
n_features = 20

# Create data with decreasing variance along dimensions
data_hd = np.random.randn(n_samples, n_features)
for i in range(n_features):
    data_hd[:, i] *= (1.0 / (i + 1))  # Decrease variance

# Fit PCA with all components
pca_full = dr.PCA(n_components=0, center=True, scale=False)
pca_full.fit(data_hd)

explained_var = pca_full.get_explained_variance_ratio()
cumulative_var = np.cumsum(explained_var)

print(f"Total components: {len(explained_var)}")
print(f"Variance explained by first 5 components: {cumulative_var[4]:.2%}")
print(f"Variance explained by first 10 components: {cumulative_var[9]:.2%}")

# Create scree plot
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

# Individual explained variance
ax1.bar(range(1, len(explained_var) + 1), explained_var, alpha=0.7, color='steelblue')
ax1.plot(range(1, len(explained_var) + 1), explained_var, 'o-', color='red', linewidth=2)
ax1.set_xlabel('Principal Component')
ax1.set_ylabel('Explained Variance Ratio')
ax1.set_title('Scree Plot: Variance Explained by Each Component')
ax1.grid(True, alpha=0.3)
ax1.set_xlim(0, 21)

# Cumulative explained variance
ax2.plot(range(1, len(cumulative_var) + 1), cumulative_var, 'o-', linewidth=2, markersize=6)
ax2.axhline(y=0.95, color='r', linestyle='--', label='95% threshold')
ax2.axhline(y=0.90, color='orange', linestyle='--', label='90% threshold')
ax2.set_xlabel('Number of Components')
ax2.set_ylabel('Cumulative Explained Variance')
ax2.set_title('Cumulative Explained Variance')
ax2.legend()
ax2.grid(True, alpha=0.3)
ax2.set_xlim(0, 21)
ax2.set_ylim(0, 1.05)

plt.tight_layout()
plt.savefig('pca_scree_plot.png', dpi=150, bbox_inches='tight')
print("\n✓ Saved: pca_scree_plot.png")
plt.show()

# ============================================================================
# Example 3: Effect of Scaling on PCA
# ============================================================================
print("\n3. Comparing PCA with and without Scaling")
print("-" * 70)

# Create data with vastly different scales
n_samples = 100
X_unscaled = np.random.randn(n_samples, 3)
X_unscaled[:, 0] *= 100  # First feature has large scale
X_unscaled[:, 1] *= 1    # Second feature has normal scale
X_unscaled[:, 2] *= 0.01 # Third feature has tiny scale

print("Feature scales (standard deviation):")
print(f"  Feature 1: {X_unscaled[:, 0].std():.2f}")
print(f"  Feature 2: {X_unscaled[:, 1].std():.2f}")
print(f"  Feature 3: {X_unscaled[:, 2].std():.2f}")

# PCA without scaling
pca_no_scale = dr.PCA(n_components=3, center=True, scale=False)
pca_no_scale.fit(X_unscaled)
explained_no_scale = pca_no_scale.get_explained_variance_ratio()

# PCA with scaling
pca_with_scale = dr.PCA(n_components=3, center=True, scale=True)
pca_with_scale.fit(X_unscaled)
explained_with_scale = pca_with_scale.get_explained_variance_ratio()

print("\nExplained variance ratio (without scaling):")
for i, var in enumerate(explained_no_scale):
    print(f"  PC{i+1}: {var:.2%}")

print("\nExplained variance ratio (with scaling):")
for i, var in enumerate(explained_with_scale):
    print(f"  PC{i+1}: {var:.2%}")

# Plot comparison
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

components = ['PC1', 'PC2', 'PC3']
x_pos = np.arange(len(components))

# Without scaling
ax1.bar(x_pos, explained_no_scale, alpha=0.7, color='coral')
ax1.set_xticks(x_pos)
ax1.set_xticklabels(components)
ax1.set_ylabel('Explained Variance Ratio')
ax1.set_title('PCA without Scaling\n(dominated by large-scale features)')
ax1.set_ylim(0, 1.0)
ax1.grid(True, alpha=0.3, axis='y')

# With scaling
ax2.bar(x_pos, explained_with_scale, alpha=0.7, color='steelblue')
ax2.set_xticks(x_pos)
ax2.set_xticklabels(components)
ax2.set_ylabel('Explained Variance Ratio')
ax2.set_title('PCA with Scaling\n(equal weight to all features)')
ax2.set_ylim(0, 1.0)
ax2.grid(True, alpha=0.3, axis='y')

plt.tight_layout()
plt.savefig('pca_scaling_comparison.png', dpi=150, bbox_inches='tight')
print("\n✓ Saved: pca_scaling_comparison.png")
plt.show()

# ============================================================================
# Example 4: 3D Visualization
# ============================================================================
print("\n4. 3D Visualization of PCA Components")
print("-" * 70)

# Generate 10D data with 4 clusters
n_samples_per_cluster = 40
n_features = 10

cluster_centers = [
    [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
    [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1],
    [1, -1, 1, -1, 1, -1, 1, -1, 1, -1],
    [-1, 1, -1, 1, -1, 1, -1, 1, -1, 1]
]

clusters_3d = []
labels_3d = []
for i, center in enumerate(cluster_centers):
    cluster = np.random.randn(n_samples_per_cluster, n_features) * 0.4 + center
    clusters_3d.append(cluster)
    labels_3d.extend([i] * n_samples_per_cluster)

X_3d = np.vstack(clusters_3d)
labels_3d = np.array(labels_3d)

# Reduce to 3D
pca_3d = dr.PCA(n_components=3, center=True, scale=False)
X_pca_3d = pca_3d.fit_transform(X_3d)

explained_var_3d = pca_3d.get_explained_variance_ratio()
print(f"Explained variance by 3 components: {explained_var_3d.sum():.2%}")

# Create 3D plot
fig = plt.figure(figsize=(14, 6))

# 3D scatter plot
ax1 = fig.add_subplot(121, projection='3d')
colors_3d = ['red', 'blue', 'green', 'orange']
for i in range(4):
    mask = labels_3d == i
    ax1.scatter(X_pca_3d[mask, 0], X_pca_3d[mask, 1], X_pca_3d[mask, 2],
                c=colors_3d[i], label=f'Cluster {i+1}', alpha=0.6, s=30)

ax1.set_xlabel(f'PC1 ({explained_var_3d[0]:.1%})')
ax1.set_ylabel(f'PC2 ({explained_var_3d[1]:.1%})')
ax1.set_zlabel(f'PC3 ({explained_var_3d[2]:.1%})')
ax1.set_title('3D PCA Projection')
ax1.legend()

# 2D projections
ax2 = fig.add_subplot(122)
for i in range(4):
    mask = labels_3d == i
    ax2.scatter(X_pca_3d[mask, 0], X_pca_3d[mask, 1],
                c=colors_3d[i], label=f'Cluster {i+1}', alpha=0.6, s=30)

ax2.set_xlabel(f'PC1 ({explained_var_3d[0]:.1%})')
ax2.set_ylabel(f'PC2 ({explained_var_3d[1]:.1%})')
ax2.set_title('2D Projection (PC1 vs PC2)')
ax2.legend()
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('pca_3d_visualization.png', dpi=150, bbox_inches='tight')
print("\n✓ Saved: pca_3d_visualization.png")
plt.show()

# ============================================================================
# Example 5: Principal Component Loadings
# ============================================================================
print("\n5. Visualizing Principal Component Loadings")
print("-" * 70)

# Use the previous 10D data
components = pca_3d.get_components()

print(f"Components shape: {components.shape}")
print("Each column represents the loadings for one principal component")

# Plot heatmap of loadings
fig, axes = plt.subplots(1, 3, figsize=(15, 4))

for i in range(3):
    loadings = components[:, i]
    axes[i].bar(range(len(loadings)), loadings, alpha=0.7, color='steelblue')
    axes[i].axhline(y=0, color='black', linestyle='-', linewidth=0.5)
    axes[i].set_xlabel('Feature Index')
    axes[i].set_ylabel('Loading')
    axes[i].set_title(f'PC{i+1} Loadings ({explained_var_3d[i]:.1%} variance)')
    axes[i].grid(True, alpha=0.3, axis='y')
    axes[i].set_xticks(range(n_features))

plt.tight_layout()
plt.savefig('pca_loadings.png', dpi=150, bbox_inches='tight')
print("\n✓ Saved: pca_loadings.png")
plt.show()

# ============================================================================
# Example 6: Reconstruction Error vs Number of Components
# ============================================================================
print("\n6. Reconstruction Error Analysis")
print("-" * 70)

# Test different numbers of components
max_components = min(15, X_3d.shape[1])
reconstruction_errors = []

for n_comp in range(1, max_components + 1):
    pca_temp = dr.PCA(n_components=n_comp, center=True, scale=False)
    X_transformed = pca_temp.fit_transform(X_3d)
    X_reconstructed = pca_temp.inverse_transform(X_transformed)
    
    error = np.linalg.norm(X_3d - X_reconstructed) / np.linalg.norm(X_3d)
    reconstruction_errors.append(error)
    
    if n_comp in [1, 3, 5, 10]:
        print(f"  {n_comp} components: {error:.4f} relative error ({(1-error)*100:.1f}% preserved)")

# Plot reconstruction error
plt.figure(figsize=(10, 6))
plt.plot(range(1, max_components + 1), reconstruction_errors, 'o-', linewidth=2, markersize=8)
plt.xlabel('Number of Components')
plt.ylabel('Relative Reconstruction Error')
plt.title('PCA Reconstruction Error vs Number of Components')
plt.grid(True, alpha=0.3)
plt.xticks(range(1, max_components + 1))

plt.tight_layout()
plt.savefig('pca_reconstruction_error.png', dpi=150, bbox_inches='tight')
print("\n✓ Saved: pca_reconstruction_error.png")
plt.show()

print("\n" + "=" * 70)
print("All examples completed successfully!")
print("=" * 70)
print("\nGenerated files:")
print("  - pca_2d_visualization.png")
print("  - pca_scree_plot.png")
print("  - pca_scaling_comparison.png")
print("  - pca_3d_visualization.png")
print("  - pca_loadings.png")
print("  - pca_reconstruction_error.png")
print("\nKey takeaways:")
print("  1. PCA effectively reduces dimensionality while preserving variance")
print("  2. Use scree plots to determine optimal number of components")
print("  3. Scale your data when features have different units/scales")
print("  4. Principal components reveal the main axes of variation")
print("  5. Reconstruction error decreases with more components")
