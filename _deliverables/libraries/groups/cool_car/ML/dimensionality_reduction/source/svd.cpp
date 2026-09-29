#include "svd.h"
#include "lib_metadata.h"
#include <algorithm>
#include <cmath>
#include <limits>
LIBRARY_METADATA(svd, "SVD", "1.0.0", "Singular Value Decomposition", "CoolBox");

namespace dimensionality_reduction {

SVD::SVD(bool compute_full_matrices)
    : compute_full_matrices_(compute_full_matrices), computed_(false) {}

void SVD::compute(const Matrix& X) {
    const auto options = compute_full_matrices_
        ? (Eigen::ComputeFullU | Eigen::ComputeFullV)
        : (Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(X.data, options);

    U_ = Matrix(svd.matrixU());
    V_ = Matrix(svd.matrixV());
    const Eigen::VectorXd& sv = svd.singularValues();
    S_.assign(sv.data(), sv.data() + sv.size());
    computed_ = true;
}

SVD::Matrix SVD::get_U() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    return U_;
}

SVD::Vector SVD::get_singular_values() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    return S_;
}

SVD::Matrix SVD::get_V() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    return V_;
}

SVD::Matrix SVD::get_S() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    const auto n = static_cast<int>(S_.size());
    Matrix S = Matrix::Zero(n, n);
    for (int i = 0; i < n; ++i) {
        S(i, i) = S_[static_cast<std::size_t>(i)];
    }
    return S;
}

SVD::Matrix SVD::reconstruct(int num_components) const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    const int available = static_cast<int>(S_.size());
    const int k = (num_components <= 0 || num_components > available) ? available : num_components;

    Eigen::MatrixXd U_k = U_.data.leftCols(k);
    Eigen::MatrixXd V_k = V_.data.leftCols(k);
    Eigen::VectorXd s_k(k);
    for (int i = 0; i < k; ++i) {
        s_k(i) = S_[static_cast<std::size_t>(i)];
    }
    return Matrix(U_k * s_k.asDiagonal() * V_k.transpose());
}

int SVD::rank(double tolerance) const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    if (S_.empty()) return 0;
    double tol = tolerance;
    if (tol < 0.0) {
        const int max_dim = std::max(U_.rows(), V_.rows());
        tol = static_cast<double>(max_dim) * S_[0] * std::numeric_limits<double>::epsilon();
    }
    int count = 0;
    for (const double s : S_) {
        if (s > tol) ++count;
    }
    return count;
}

SVD::Vector SVD::explained_variance_ratio() const {
    if (!computed_) throw std::runtime_error("SVD not computed yet");
    double total = 0.0;
    for (const double s : S_) total += s * s;
    Vector ratio(S_.size(), 0.0);
    if (total > 0.0) {
        for (std::size_t i = 0; i < S_.size(); ++i) {
            ratio[i] = (S_[i] * S_[i]) / total;
        }
    }
    return ratio;
}

} // namespace dimensionality_reduction
