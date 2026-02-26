#include "pca.h"
#include <stdexcept>

#include "lib_metadata.h"
LIBRARY_METADATA(pca, "PCA", "1.0.0", "Principal Component Analysis", "CoolBox");

namespace dimensionality_reduction {

PCA::PCA(int n_components, bool center, bool scale)
    : n_components_(n_components), center_(center), scale_data_(scale), fitted_(false) {}

Eigen::VectorXd PCA::compute_mean(const Eigen::MatrixXd& X) {
    return X.colwise().mean();
}

Eigen::VectorXd PCA::compute_std(const Eigen::MatrixXd& X, const Eigen::VectorXd& mean) {
    Eigen::MatrixXd centered = X.rowwise() - mean.transpose();
    Eigen::VectorXd variance = (centered.array().square().colwise().sum() / (X.rows() - 1)).matrix();
    return variance.array().sqrt().matrix();
}

Eigen::MatrixXd PCA::preprocess(const Eigen::MatrixXd& X) const {
    Eigen::MatrixXd result = X;
    if (center_) {
        result = result.rowwise() - mean_.transpose();
    }
    if (scale_data_ && scale_.size() > 0) {
        for (int j = 0; j < result.cols(); ++j) {
            if (scale_(j) > 1e-10) {
                result.col(j) /= scale_(j);
            }
        }
    }
    return result;
}

void PCA::fit(const Eigen::MatrixXd& X) {
    int n_samples = X.rows();
    int n_features = X.cols();

    mean_ = compute_mean(X);
    scale_ = compute_std(X, mean_);

    Eigen::MatrixXd X_proc = preprocess(X);

    svd_.compute(X_proc);

    singular_values_ = svd_.get_singular_values();
    Eigen::MatrixXd V = svd_.get_V();

    // Determine number of components
    if (n_components_ <= 0) {
        n_components_ = std::min(n_samples, n_features);
    }
    n_components_ = std::min(n_components_, std::min(n_samples, n_features));

    components_ = V.leftCols(n_components_);

    // Explained variance
    explained_variance_ = (singular_values_.head(n_components_).array().square() / (n_samples - 1)).matrix();
    double total_var = (singular_values_.array().square() / (n_samples - 1)).sum();
    explained_variance_ratio_ = explained_variance_ / total_var;

    fitted_ = true;
}

Eigen::MatrixXd PCA::transform(const Eigen::MatrixXd& X) const {
    if (!fitted_) throw std::runtime_error("PCA not fitted yet");
    Eigen::MatrixXd X_proc = preprocess(X);
    return X_proc * components_;
}

Eigen::MatrixXd PCA::fit_transform(const Eigen::MatrixXd& X) {
    fit(X);
    return transform(X);
}

Eigen::MatrixXd PCA::inverse_transform(const Eigen::MatrixXd& X_transformed) const {
    if (!fitted_) throw std::runtime_error("PCA not fitted yet");
    Eigen::MatrixXd result = X_transformed * components_.transpose();
    if (scale_data_ && scale_.size() > 0) {
        for (int j = 0; j < result.cols(); ++j) {
            if (scale_(j) > 1e-10) {
                result.col(j) *= scale_(j);
            }
        }
    }
    if (center_) {
        result = result.rowwise() + mean_.transpose();
    }
    return result;
}

Eigen::MatrixXd PCA::get_components() const {
    if (!fitted_) throw std::runtime_error("PCA not fitted yet");
    return components_;
}

Eigen::VectorXd PCA::get_explained_variance() const {
    if (!fitted_) throw std::runtime_error("PCA not fitted yet");
    return explained_variance_;
}

Eigen::VectorXd PCA::get_explained_variance_ratio() const {
    if (!fitted_) throw std::runtime_error("PCA not fitted yet");
    return explained_variance_ratio_;
}

Eigen::VectorXd PCA::get_singular_values() const {
    if (!fitted_) throw std::runtime_error("PCA not fitted yet");
    return singular_values_;
}

} // namespace dimensionality_reduction
