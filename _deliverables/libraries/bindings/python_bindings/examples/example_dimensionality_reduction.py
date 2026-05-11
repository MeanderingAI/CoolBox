"""
Example demonstrating SVD and PCA usage with ml_core
"""
import numpy as np
import ml_core.dimensionality_reduction as dr

print("=" * 60)
print("SVD Example")
print("=" * 60)

# Create a sample matrix
X = np.array([
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9],
    [10, 11, 12]
], dtype=np.float64)

print("\nOriginal matrix X:")
print(X)
print(f"Shape: {X.shape}")

# Compute SVD
svd = dr.SVD()
svd.compute(X)

# Get components
U = svd.get_U()
S = svd.get_singular_values()
V = svd.get_V()

print("\nLeft singular vectors (U):")
print(U)
print(f"Shape: {U.shape}")

print("\nSingular values (S):")
print(S)

print("\nRight singular vectors (V):")
print(V)
print(f"Shape: {V.shape}")

# Reconstruct
X_reconstructed = svd.reconstruct()
print("\nReconstructed matrix:")
print(X_reconstructed)

reconstruction_error = np.linalg.norm(X - X_reconstructed) / np.linalg.norm(X)
print(f"\nReconstruction error: {reconstruction_error:.2e}")

# Low-rank approximation
X_approx = svd.reconstruct(2)
print("\nLow-rank approximation (2 components):")
print(X_approx)

approx_error = np.linalg.norm(X - X_approx) / np.linalg.norm(X)
print(f"Approximation error: {approx_error:.4f}")

# Explained variance
explained_var = svd.explained_variance_ratio()
print("\nExplained variance ratio:")
for i, var in enumerate(explained_var):
    print(f"  Component {i+1}: {var:.4f} ({var*100:.2f}%)")

# Rank
rank = svd.rank()
print(f"\nMatrix rank: {rank}")

print("\n" + "=" * 60)
print("PCA Example")
print("=" * 60)

# Create a dataset with correlation
np.random.seed(42)
n_samples = 100
n_features = 5

# Generate correlated features
mean = np.zeros(n_features)
cov = np.random.rand(n_features, n_features)
cov = cov @ cov.T  # Make positive semi-definite
X_data = np.random.multivariate_normal(mean, cov, n_samples)

print(f"\nOriginal data shape: {X_data.shape}")
print(f"Mean: {X_data.mean(axis=0)}")
print(f"Std: {X_data.std(axis=0)}")

# Fit PCA with 3 components
n_components = 3
pca = dr.PCA(n_components=n_components, center=True, scale=False)
pca.fit(X_data)

print(f"\nFitted PCA with {pca.get_n_components()} components")

# Transform data
X_transformed = pca.transform(X_data)
print(f"\nTransformed data shape: {X_transformed.shape}")
print(f"Transformed mean: {X_transformed.mean(axis=0)}")

# Get components
components = pca.get_components()
print(f"\nPrincipal components shape: {components.shape}")

# Explained variance
explained_var = pca.get_explained_variance_ratio()
print("\nExplained variance ratio:")
for i, var in enumerate(explained_var):
    print(f"  PC{i+1}: {var:.4f} ({var*100:.2f}%)")
    
cumulative_var = np.cumsum(explained_var)
print("\nCumulative explained variance:")
for i, var in enumerate(cumulative_var):
    print(f"  PC1-{i+1}: {var:.4f} ({var*100:.2f}%)")

# Inverse transform
X_reconstructed = pca.inverse_transform(X_transformed)
print(f"\nReconstructed data shape: {X_reconstructed.shape}")

reconstruction_error = np.linalg.norm(X_data - X_reconstructed) / np.linalg.norm(X_data)
print(f"Reconstruction error: {reconstruction_error:.4f}")

# Get mean and scale
mean = pca.get_mean()
print(f"\nLearned mean: {mean}")

print("\n" + "=" * 60)
print("PCA with Scaling Example")
print("=" * 60)

# Create data with different scales
X_scaled = X_data.copy()
X_scaled[:, 0] *= 100  # Scale first feature by 100
X_scaled[:, 1] *= 0.01  # Scale second feature by 0.01

print(f"\nData with different scales:")
print(f"Std: {X_scaled.std(axis=0)}")

# PCA without scaling (dominated by large-scale features)
pca_no_scale = dr.PCA(n_components=3, center=True, scale=False)
pca_no_scale.fit(X_scaled)

print("\nPCA without scaling - explained variance:")
for i, var in enumerate(pca_no_scale.get_explained_variance_ratio()):
    print(f"  PC{i+1}: {var:.4f}")

# PCA with scaling (unit variance for all features)
pca_with_scale = dr.PCA(n_components=3, center=True, scale=True)
pca_with_scale.fit(X_scaled)

print("\nPCA with scaling - explained variance:")
for i, var in enumerate(pca_with_scale.get_explained_variance_ratio()):
    print(f"  PC{i+1}: {var:.4f}")

scale = pca_with_scale.get_scale()
print(f"\nLearned scale (std): {scale}")

print("\n" + "=" * 60)
print("Dimensionality Reduction for Visualization")
print("=" * 60)

# Reduce to 2D for visualization
pca_2d = dr.PCA(n_components=2, center=True, scale=True)
X_2d = pca_2d.fit_transform(X_data)

print(f"\nReduced to 2D: {X_2d.shape}")
print(f"Explained variance: {pca_2d.get_explained_variance_ratio().sum():.4f}")
print(f"This captures {pca_2d.get_explained_variance_ratio().sum()*100:.2f}% of total variance")

print("\n✓ All examples completed successfully!")
