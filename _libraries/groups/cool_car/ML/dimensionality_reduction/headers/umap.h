
#ifndef UMAP_H
#define UMAP_H

// #include "lib_metadata.h"
// Metadata only defined in umap.cpp to avoid duplicate symbols


#include "mytrix_eigen_compat.hpp"
#include "matrix_dense.h"
#include "knn.h"
#include <vector>
#include <random>

namespace dimensionality_reduction {

// Uniform Manifold Approximation and Projection (UMAP)
// Dimensionality reduction technique that preserves both local and global structure.
// Based on manifold learning and topological data analysis.
// Reference: McInnes, L., Healy, J., & Melville, J. (2018).
// UMAP: Uniform Manifold Approximation and Projection for Dimension Reduction.
// arXiv preprint arXiv:1802.03426.
class UMAP {
public:
    UMAP(int n_components = 2, int n_neighbors = 15, double min_dist = 0.1,
         const std::string& metric = "euclidean", double learning_rate = 1.0,
         int n_epochs = 200, int random_state = 42);
    void fit(const matrix::DenseMatrix& X);
    matrix::DenseMatrix transform(const matrix::DenseMatrix& X) const;
    matrix::DenseMatrix fit_transform(const matrix::DenseMatrix& X);
    matrix::DenseMatrix get_embedding() const;
    bool is_fitted() const { return fitted_; }
    int get_n_components() const { return n_components_; }
    
    // Get number of neighbors
    int get_n_neighbors() const { return n_neighbors_; }
    
private:
    // Hyperparameters
    int n_components_;
    int n_neighbors_;
    double min_dist_;
    std::string metric_;
    double learning_rate_;
    int n_epochs_;
    int random_state_;

    // State
    bool fitted_;
    matrix::DenseMatrix X_train_;
    matrix::DenseMatrix embedding_;
    KNN knn_;

    // Random number generator
    std::mt19937 rng_;

    // UMAP parameters
    double a_;  // Curve parameter
    double b_;  // Curve parameter

    // Compute fuzzy simplicial set (high-dimensional graph)
    std::pair<std::vector<std::vector<int>>, matrix::DenseMatrix> compute_membership_strengths(
        const std::vector<std::vector<int>>& knn_indices,
        const matrix::DenseMatrix& knn_distances);

    // Compute smooth kNN distances (local connectivity)
    std::vector<double> smooth_knn_dist(
        const matrix::DenseMatrix& distances,
        int k,
        int n_iter = 64,
        double local_connectivity = 1.0,
        double bandwidth = 1.0);

    // Initialize embedding using spectral method or random
    matrix::DenseMatrix initialize_embedding(int n_samples);

    // Optimize embedding using stochastic gradient descent
    void optimize_embedding(
        const matrix::DenseMatrix& graph_weights,
        const std::vector<std::vector<int>>& graph_edges);

    // Compute a and b parameters from min_dist
    void find_ab_params();

    // UMAP loss function (attractive + repulsive forces)
    double compute_loss(
        const std::vector<double>& y_i,
        const std::vector<double>& y_j,
        double weight) const;

    // Gradient of UMAP loss
    std::vector<double> compute_gradient(
        const std::vector<double>& y_i,
        const std::vector<double>& y_j,
        double weight,
        bool attractive) const;
};

} // namespace dimensionality_reduction

#endif // UMAP_H
