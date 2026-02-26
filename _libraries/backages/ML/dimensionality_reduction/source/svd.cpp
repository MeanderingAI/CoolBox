#include "svd.h"

#include "lib_metadata.h"
LIBRARY_METADATA(svd, "SVD", "1.0.0", "Singular Value Decomposition", "CoolBox");

namespace dimensionality_reduction {

SVD::SVD(bool compute_full_matrices)
    : compute_full_matrices_(compute_full_matrices), computed_(false) {}

void SVD::compute(const Eigen::MatrixXd& X) {
    unsigned int options = compute_full_matrices_ ?
        (Eigen::ComputeFullU | Eigen::ComputeFullV) :
        (Eigen::ComputeThinU | Eigen::ComputeThinV);
    svd_.compute(X, options);
    U_ = svd_.matrixU();
    S_ = svd_.singularValues();
    V_ = svd_.matrixV();
    computed_ = true;
}

Eigen::MatrixXd SVD::get_U() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    return U_;
}

Eigen::VectorXd SVD::get_singular_values() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    return S_;
}

Eigen::MatrixXd SVD::get_V() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    return V_;
}

Eigen::MatrixXd SVD::get_S() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    return S_.asDiagonal();
}

Eigen::MatrixXd SVD::reconstruct(int num_components) const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    int k = (num_components > 0) ? num_components : static_cast<int>(S_.size());
    return U_.leftCols(k) * S_.head(k).asDiagonal() * V_.leftCols(k).transpose();
}

int SVD::rank(double tolerance) const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    double tol = (tolerance < 0) ? S_(0) * std::max(U_.rows(), V_.rows()) * 1e-12 : tolerance;
    int r = 0;
    for (int i = 0; i < S_.size(); ++i) {
        if (S_(i) > tol) r++;
    }
    return r;
}

Eigen::VectorXd SVD::explained_variance_ratio() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    Eigen::VectorXd sq = S_.array().square();
    double total = sq.sum();
    if (total < 1e-15) return Eigen::VectorXd::Zero(S_.size());
    return sq / total;
}

} // namespace dimensionality_reduction
