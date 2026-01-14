
#ifndef UMAP_H
#define UMAP_H

#include "lib_metadata.h"
// Metadata only defined in umap.cpp to avoid duplicate symbols

#include <Eigen/Dense>
#include "ML/dimensionality_reduction/knn.h"
#include <vector>
#include <random>

namespace dimensionality_reduction {

/**
 * @brief Uniform Manifold Approximation and Projection (UMAP)
 * 
 * Dimensionality reduction technique that preserves both local and global structure.
 * Based on manifold learning and topological data analysis.
 * 
 * Reference: McInnes, L., Healy, J., & Melville, J. (2018).
 * UMAP: Uniform Manifold Approximation and Projection for Dimension Reduction.
 * arXiv preprint arXiv:1802.03426.
 */
class UMAP {
public:
    /**
     * @brief Construct UMAP object
     * @param n_components Number of dimensions in the embedding
     * @param n_neighbors Number of nearest neighbors to consider
     * @param min_dist Minimum distance between points in embedding
     * @param metric Distance metric ("euclidean", "manhattan", "cosine")
     * @param learning_rate Learning rate for optimization
     * @param n_epochs Number of optimization epochs
     * @param random_state Random seed for reproducibility
     */
    explicit UMAP(
        int n_components = 2,
        int n_neighbors = 15,
        double min_dist = 0.1,
        const std::string& metric = "euclidean",
        double learning_rate = 1.0,
        int n_epochs = 200,
        int random_state = 42
    );
    
    /**
     * @brief Fit UMAP to data
     * @param X Input data (rows = samples, cols = features)
     */
    void fit(const Eigen::MatrixXd& X);
    
    /**
     * @brief Transform data to low-dimensional embedding
     * @param X Data to transform
     * @return Embedded data
     */
    Eigen::MatrixXd transform(const Eigen::MatrixXd& X) const;
    
    /**
     * @brief Fit and transform in one step
     * @param X Input data
     * @return Embedded data
     */
    Eigen::MatrixXd fit_transform(const Eigen::MatrixXd& X);
    
    /**
     * @brief Get the learned embedding
     * @return Embedding matrix
     */
    Eigen::MatrixXd get_embedding() const;
    
    /**
     * @brief Check if UMAP has been fitted
     */
    bool is_fitted() const { return fitted_; }
    
    /**
     * @brief Get number of components
     */
    int get_n_components() const { return n_components_; }
    
    /**
     * @brief Get number of neighbors
     */
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
    Eigen::MatrixXd X_train_;
    Eigen::MatrixXd embedding_;
    KNN knn_;
    
    // Random number generator
    std::mt19937 rng_;
    
    // UMAP parameters
    double a_;  // Curve parameter
    double b_;  // Curve parameter
    
    /**
     * @brief Compute fuzzy simplicial set (high-dimensional graph)
     */
    std::pair<Eigen::MatrixXi, Eigen::MatrixXd> compute_membership_strengths(
        const Eigen::MatrixXi& knn_indices,
        const Eigen::MatrixXd& knn_distances);
    
    /**
     * @brief Compute smooth kNN distances (local connectivity)
     */
    Eigen::VectorXd smooth_knn_dist(
        const Eigen::MatrixXd& distances,
        int k,
        int n_iter = 64,
        double local_connectivity = 1.0,
        double bandwidth = 1.0);
    
    /**
     * @brief Initialize embedding using spectral method or random
     */
    Eigen::MatrixXd initialize_embedding(int n_samples);
    
    /**
     * @brief Optimize embedding using stochastic gradient descent
     */
    void optimize_embedding(
        const Eigen::MatrixXd& graph_weights,
        const Eigen::MatrixXi& graph_edges);
    
    /**
     * @brief Compute a and b parameters from min_dist
     */
    void find_ab_params();
    
    /**
     * @brief UMAP loss function (attractive + repulsive forces)
     */
    double compute_loss(
        const Eigen::VectorXd& y_i,
        const Eigen::VectorXd& y_j,
        double weight) const;
    
    /**
     * @brief Gradient of UMAP loss
     */
    Eigen::VectorXd compute_gradient(
        const Eigen::VectorXd& y_i,
        const Eigen::VectorXd& y_j,
        double weight,
        bool attractive) const;
};

} // namespace dimensionality_reduction

#endif // UMAP_H
