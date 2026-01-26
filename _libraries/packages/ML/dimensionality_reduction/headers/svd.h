
#ifndef SVD_H
#define SVD_H

#include "lib_metadata.h"
// Metadata only defined in svd.cpp to avoid duplicate symbols

#include <Eigen/Dense>
#include <Eigen/SVD>
#include <stdexcept>

namespace dimensionality_reduction {

/**
 * @brief Singular Value Decomposition wrapper for Eigen
 * 
 * Provides convenient interface for computing SVD and accessing components.
 * Uses Eigen::BDCSVD (Bidiagonal Divide and Conquer) for efficiency.
 */
class SVD {
public:
    /**
     * @brief Construct SVD object
     * @param compute_full_matrices If true, compute full U and V matrices
     */
    explicit SVD(bool compute_full_matrices = false);
    
    /**
     * @brief Compute SVD of matrix X
     * @param X Input matrix (m x n)
     * @throws std::runtime_error if decomposition fails
     */
    void compute(const Eigen::MatrixXd& X);
    
    /**
     * @brief Get left singular vectors (U matrix)
     * @return U matrix (m x k) where k = min(m,n) for thin SVD
     */
    Eigen::MatrixXd get_U() const;
    
    /**
     * @brief Get singular values as vector
     * @return Vector of singular values in descending order
     */
    Eigen::VectorXd get_singular_values() const;
    
    /**
     * @brief Get right singular vectors (V matrix)
     * @return V matrix (n x k) where k = min(m,n) for thin SVD
     */
    Eigen::MatrixXd get_V() const;
    
    /**
     * @brief Get singular values as diagonal matrix
     * @return Diagonal matrix of singular values
     */
    Eigen::MatrixXd get_S() const;
    
    /**
     * @brief Reconstruct matrix from SVD components
     * @param num_components Number of components to use (0 = all)
     * @return Reconstructed matrix
     */
    Eigen::MatrixXd reconstruct(int num_components = 0) const;
    
    /**
     * @brief Get rank of matrix
     * @param tolerance Tolerance for considering singular values as zero
     * @return Estimated rank
     */
    int rank(double tolerance = -1.0) const;
    
    /**
     * @brief Get explained variance ratio for each component
     * @return Vector of explained variance ratios
     */
    Eigen::VectorXd explained_variance_ratio() const;
    
    /**
     * @brief Check if SVD has been computed
     * @return true if compute() has been called
     */
    bool is_computed() const { return computed_; }
    
private:
    bool compute_full_matrices_;
    bool computed_;
    Eigen::BDCSVD<Eigen::MatrixXd> svd_;
    Eigen::MatrixXd U_;
    Eigen::VectorXd S_;
    Eigen::MatrixXd V_;
};

} // namespace dimensionality_reduction

#endif // SVD_H
