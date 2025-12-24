#ifndef KNN_H
#define KNN_H

#include <Eigen/Dense>
#include <vector>
#include <queue>
#include <functional>
#include <limits>

namespace dimensionality_reduction {

/**
 * @brief k-Nearest Neighbors search implementation
 * 
 * Provides efficient nearest neighbor search for UMAP and other algorithms.
 * Uses brute-force search (can be optimized with KD-tree for larger datasets).
 */
class KNN {
public:
    /**
     * @brief Construct KNN object
     * @param k Number of nearest neighbors to find
     * @param metric Distance metric ("euclidean", "manhattan", "cosine")
     */
    explicit KNN(int k = 5, const std::string& metric = "euclidean");
    
    /**
     * @brief Fit the KNN model with training data
     * @param X Training data matrix (rows = samples, cols = features)
     */
    void fit(const Eigen::MatrixXd& X);
    
    /**
     * @brief Find k-nearest neighbors for query points
     * @param X_query Query points (rows = samples, cols = features)
     * @return Pair of (indices, distances) matrices
     */
    std::pair<Eigen::MatrixXi, Eigen::MatrixXd> kneighbors(
        const Eigen::MatrixXd& X_query) const;
    
    /**
     * @brief Find k-nearest neighbors for training data (exclude self)
     * @return Pair of (indices, distances) matrices
     */
    std::pair<Eigen::MatrixXi, Eigen::MatrixXd> kneighbors() const;
    
    /**
     * @brief Compute pairwise distances between two sets of points
     * @param X First set of points
     * @param Y Second set of points
     * @return Distance matrix
     */
    Eigen::MatrixXd pairwise_distances(
        const Eigen::MatrixXd& X, 
        const Eigen::MatrixXd& Y) const;
    
    /**
     * @brief Get number of neighbors
     */
    int get_k() const { return k_; }
    
    /**
     * @brief Get distance metric
     */
    std::string get_metric() const { return metric_; }
    
    /**
     * @brief Check if KNN has been fitted
     */
    bool is_fitted() const { return fitted_; }
    
private:
    int k_;
    std::string metric_;
    bool fitted_;
    Eigen::MatrixXd X_train_;
    
    /**
     * @brief Compute distance between two vectors
     */
    double compute_distance(
        const Eigen::VectorXd& x1, 
        const Eigen::VectorXd& x2) const;
    
    /**
     * @brief Find k nearest neighbors for a single point
     */
    std::pair<std::vector<int>, std::vector<double>> find_neighbors_single(
        const Eigen::VectorXd& query, 
        bool exclude_self = false,
        int self_index = -1) const;
};

} // namespace dimensionality_reduction

#endif // KNN_H
