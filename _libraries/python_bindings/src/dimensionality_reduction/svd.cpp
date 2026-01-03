#include "ml/dimensionality_reduction/svd.h"
#include <iostream>

namespace dimensionality_reduction {

SVD::SVD(bool compute_full_matrices)
    : compute_full_matrices_(compute_full_matrices), computed_(false) {}

void SVD::compute(const Eigen::MatrixXd& X) {
    if (X.rows() == 0 || X.cols() == 0) {
        throw std::runtime_error("Cannot compute SVD of empty matrix");
    }
    
    unsigned int compute_options = compute_full_matrices_ 
        ? (Eigen::ComputeFullU | Eigen::ComputeFullV)
        : (Eigen::ComputeThinU | Eigen::ComputeThinV);
    
    svd_.compute(X, compute_options);
    
    U_ = svd_.matrixU();
    S_ = svd_.singularValues();
    V_ = svd_.matrixV();
    
    computed_ = true;
}

Eigen::MatrixXd SVD::get_U() const {
    if (!computed_) {
        throw std::runtime_error("SVD not computed yet. Call compute() first.");
    }
    return U_;
}

Eigen::VectorXd SVD::get_singular_values() const {
    if (!computed_) {
        throw std::runtime_error("SVD not computed yet. Call compute() first.");
    }
    return S_;
}

Eigen::MatrixXd SVD::get_V() const {
    if (!computed_) {
        throw std::runtime_error("SVD not computed yet. Call compute() first.");
    }
    return V_;
}

Eigen::MatrixXd SVD::get_S() const {
    if (!computed_) {
        throw std::runtime_error("SVD not computed yet. Call compute() first.");
    }
    return S_.asDiagonal();
}

Eigen::MatrixXd SVD::reconstruct(int num_components) const {
    if (!computed_) {
        throw std::runtime_error("SVD not computed yet. Call compute() first.");
    }
    
    int k = num_components;
    if (k <= 0 || k > S_.size()) {
        k = S_.size();
    }
    
    // X_approx = U[:, :k] * S[:k, :k] * V[:, :k]^T
    Eigen::MatrixXd U_k = U_.leftCols(k);
    Eigen::VectorXd S_k = S_.head(k);
    Eigen::MatrixXd V_k = V_.leftCols(k);
    
    return U_k * S_k.asDiagonal() * V_k.transpose();
}

int SVD::rank(double tolerance) const {
    if (!computed_) {
        throw std::runtime_error("SVD not computed yet. Call compute() first.");
    }
    
    double tol = tolerance;
    if (tol < 0.0) {
        // Use machine epsilon scaled by largest dimension and max singular value
        tol = std::max(U_.rows(), V_.rows()) * S_(0) * std::numeric_limits<double>::epsilon();
    }
    
    int r = 0;
    for (int i = 0; i < S_.size(); ++i) {
        if (S_(i) > tol) {
            r++;
        }
    }
    return r;
}

Eigen::VectorXd SVD::explained_variance_ratio() const {
    if (!computed_) {
        throw std::runtime_error("SVD not computed yet. Call compute() first.");
    }
    
    // Variance explained by each component = (singular_value^2) / sum(all singular_values^2)
    Eigen::VectorXd S_squared = S_.array().square();
    double total_variance = S_squared.sum();
    
    if (total_variance == 0.0) {
        return Eigen::VectorXd::Zero(S_.size());
    }
    
    return S_squared / total_variance;
}

} // namespace dimensionality_reduction
