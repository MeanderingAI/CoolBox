#include "lib_metadata.h"

LIBRARY_METADATA(
    pca,
    "PCA",
    "1.0",
    "Principal Component Analysis for dimensionality reduction and feature extraction.",
    "ToolBox Authors"
)
#include "ml/dimensionality_reduction/pca.h"
#include <iostream>
#include <cmath>

// Library metadata
extern "C" {
    __attribute__((visibility("default"), used))
    const char* get_library_name() { return "dimensionality_reduction"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_version() { return "2.0.0"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_description() { return "Dimensionality reduction algorithms: PCA, SVD, t-SNE, and UMAP for machine learning"; }
    
    __attribute__((visibility("default"), used))
    const char* get_library_author() { return "ToolBox ML Team"; }
}

namespace dimensionality_reduction {

PCA::PCA(int n_components, bool center, bool scale)
    : n_components_(n_components), center_(center), scale_data_(scale), fitted_(false) {}

Eigen::VectorXd PCA::compute_mean(const Eigen::MatrixXd& X) {
    return X.colwise().mean();
}

Eigen::VectorXd PCA::compute_std(const Eigen::MatrixXd& X, const Eigen::VectorXd& mean) {
    int n = X.rows();
    if (n <= 1) {
        return Eigen::VectorXd::Ones(X.cols());
    }
    
    Eigen::MatrixXd centered = X.rowwise() - mean.transpose();
    Eigen::VectorXd variance = (centered.array().square().colwise().sum()) / (n - 1);
    
    // Avoid division by zero
    Eigen::VectorXd std_dev = variance.array().sqrt();
    for (int i = 0; i < std_dev.size(); ++i) {
        if (std_dev(i) < 1e-10) {
            std_dev(i) = 1.0;
        }
    }
    
    return std_dev;
}

Eigen::MatrixXd PCA::preprocess(const Eigen::MatrixXd& X) const {
    if (!fitted_) {
        throw std::runtime_error("PCA not fitted yet. Call fit() first.");
    }
    
    Eigen::MatrixXd X_processed = X;
    
    if (center_) {
        X_processed = X_processed.rowwise() - mean_.transpose();
    }
    
    if (scale_data_) {
        X_processed = X_processed.array().rowwise() / scale_.transpose().array();
    }
    
    return X_processed;
}

void PCA::fit(const Eigen::MatrixXd& X) {
    if (X.rows() == 0 || X.cols() == 0) {
        throw std::runtime_error("Cannot fit PCA on empty matrix");
    }
    
    // Compute statistics
    if (center_) {
        mean_ = compute_mean(X);
    } else {
        mean_ = Eigen::VectorXd::Zero(X.cols());
    }
    
    if (scale_data_) {
        scale_ = compute_std(X, mean_);
    } else {
        scale_ = Eigen::VectorXd::Ones(X.cols());
    }
    
    // Center and scale data
    Eigen::MatrixXd X_centered = X.rowwise() - mean_.transpose();
    if (scale_data_) {
        X_centered = X_centered.array().rowwise() / scale_.transpose().array();
    }
    
    // Compute SVD
    svd_.compute(X_centered);
    
    // Get components
    Eigen::MatrixXd V = svd_.get_V();
    Eigen::VectorXd S = svd_.get_singular_values();
    
    // Determine number of components to keep
    int k = n_components_;
    if (k <= 0 || k > V.cols()) {
        k = V.cols();
    }
    n_components_ = k;
    
    // Store principal components (right singular vectors)
    components_ = V.leftCols(k);
    singular_values_ = S.head(k);
    
    // Compute explained variance
    // Variance = (singular_values^2) / (n_samples - 1)
    int n_samples = X.rows();
    Eigen::VectorXd S_squared = singular_values_.array().square();
    explained_variance_ = S_squared / (n_samples - 1);
    
    // Compute explained variance ratio
    double total_variance = S_squared.sum() / (n_samples - 1);
    if (total_variance > 0.0) {
        explained_variance_ratio_ = explained_variance_ / total_variance;
    } else {
        explained_variance_ratio_ = Eigen::VectorXd::Zero(k);
    }
    
    fitted_ = true;
}

Eigen::MatrixXd PCA::transform(const Eigen::MatrixXd& X) const {
    if (!fitted_) {
        throw std::runtime_error("PCA not fitted yet. Call fit() first.");
    }
    
    if (X.cols() != mean_.size()) {
        throw std::runtime_error("Number of features in X does not match fitted data");
    }
    
    // Preprocess and project onto principal components
    Eigen::MatrixXd X_processed = preprocess(X);
    return X_processed * components_;
}

Eigen::MatrixXd PCA::fit_transform(const Eigen::MatrixXd& X) {
    fit(X);
    return transform(X);
}

Eigen::MatrixXd PCA::inverse_transform(const Eigen::MatrixXd& X_transformed) const {
    if (!fitted_) {
        throw std::runtime_error("PCA not fitted yet. Call fit() first.");
    }
    
    if (X_transformed.cols() != n_components_) {
        throw std::runtime_error("Number of components in X_transformed does not match n_components");
    }
    
    // Project back to original space
    Eigen::MatrixXd X_reconstructed = X_transformed * components_.transpose();
    
    // Undo scaling and centering
    if (scale_data_) {
        X_reconstructed = X_reconstructed.array().rowwise() * scale_.transpose().array();
    }
    
    if (center_) {
        X_reconstructed = X_reconstructed.rowwise() + mean_.transpose();
    }
    
    return X_reconstructed;
}

Eigen::MatrixXd PCA::get_components() const {
    if (!fitted_) {
        throw std::runtime_error("PCA not fitted yet. Call fit() first.");
    }
    return components_;
}

Eigen::VectorXd PCA::get_explained_variance() const {
    if (!fitted_) {
        throw std::runtime_error("PCA not fitted yet. Call fit() first.");
    }
    return explained_variance_;
}

Eigen::VectorXd PCA::get_explained_variance_ratio() const {
    if (!fitted_) {
        throw std::runtime_error("PCA not fitted yet. Call fit() first.");
    }
    return explained_variance_ratio_;
}

Eigen::VectorXd PCA::get_singular_values() const {
    if (!fitted_) {
        throw std::runtime_error("PCA not fitted yet. Call fit() first.");
    }
    return singular_values_;
}

} // namespace dimensionality_reduction
