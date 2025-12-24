#ifndef PCA_H
#define PCA_H

#include <Eigen/Dense>
#include "ml/dimensionality_reduction/svd.h"

namespace dimensionality_reduction {

/**
 * @brief Principal Component Analysis
 * 
 * Performs PCA via SVD for dimensionality reduction and feature extraction.
 * Automatically centers the data before decomposition.
 */
class PCA {
public:
    /**
     * @brief Construct PCA object
     * @param n_components Number of components to keep (0 = keep all)
     * @param center If true, center data by subtracting mean
     * @param scale If true, scale data to unit variance
     */
    explicit PCA(int n_components = 0, bool center = true, bool scale = false);
    
    /**
     * @brief Fit PCA to data matrix X
     * @param X Data matrix (rows = samples, cols = features)
     */
    void fit(const Eigen::MatrixXd& X);
    
    /**
     * @brief Transform data to principal component space
     * @param X Data matrix to transform
     * @return Transformed data (rows = samples, cols = n_components)
     */
    Eigen::MatrixXd transform(const Eigen::MatrixXd& X) const;
    
    /**
     * @brief Fit and transform in one step
     * @param X Data matrix
     * @return Transformed data
     */
    Eigen::MatrixXd fit_transform(const Eigen::MatrixXd& X);
    
    /**
     * @brief Inverse transform from PC space back to original space
     * @param X_transformed Data in PC space
     * @return Reconstructed data in original space
     */
    Eigen::MatrixXd inverse_transform(const Eigen::MatrixXd& X_transformed) const;
    
    /**
     * @brief Get principal components (loadings)
     * @return Matrix where each column is a principal component
     */
    Eigen::MatrixXd get_components() const;
    
    /**
     * @brief Get explained variance for each component
     * @return Vector of explained variances
     */
    Eigen::VectorXd get_explained_variance() const;
    
    /**
     * @brief Get explained variance ratio for each component
     * @return Vector of explained variance ratios (sum to 1.0)
     */
    Eigen::VectorXd get_explained_variance_ratio() const;
    
    /**
     * @brief Get singular values
     * @return Vector of singular values
     */
    Eigen::VectorXd get_singular_values() const;
    
    /**
     * @brief Get mean of training data
     * @return Mean vector
     */
    Eigen::VectorXd get_mean() const { return mean_; }
    
    /**
     * @brief Get standard deviation of training data
     * @return Standard deviation vector
     */
    Eigen::VectorXd get_scale() const { return scale_; }
    
    /**
     * @brief Get number of components
     * @return Number of components kept
     */
    int get_n_components() const { return n_components_; }
    
    /**
     * @brief Check if PCA has been fitted
     * @return true if fit() has been called
     */
    bool is_fitted() const { return fitted_; }
    
private:
    int n_components_;
    bool center_;
    bool scale_data_;
    bool fitted_;
    
    Eigen::VectorXd mean_;
    Eigen::VectorXd scale_;
    Eigen::MatrixXd components_;
    Eigen::VectorXd explained_variance_;
    Eigen::VectorXd explained_variance_ratio_;
    Eigen::VectorXd singular_values_;
    
    SVD svd_;
    
    /**
     * @brief Preprocess data (center and optionally scale)
     */
    Eigen::MatrixXd preprocess(const Eigen::MatrixXd& X) const;
    
    /**
     * @brief Compute mean of each column
     */
    static Eigen::VectorXd compute_mean(const Eigen::MatrixXd& X);
    
    /**
     * @brief Compute standard deviation of each column
     */
    static Eigen::VectorXd compute_std(const Eigen::MatrixXd& X, const Eigen::VectorXd& mean);
};

} // namespace dimensionality_reduction

#endif // PCA_H
