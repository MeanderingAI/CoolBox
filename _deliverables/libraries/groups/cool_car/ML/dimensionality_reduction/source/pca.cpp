
#include "pca.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
#include "lib_metadata.h"
LIBRARY_METADATA(pca, "PCA", "1.0.0", "Principal Component Analysis", "CoolBox");

dimensionality_reduction::PCA::PCA(int n_components, bool center, bool scale)
    : n_components_(n_components), center_(center), scale_data_(scale), fitted_(false),
      mean_(), scale_(), components_(1, 1), explained_variance_(), explained_variance_ratio_(), singular_values_(), svd_() {}


// Static helper implementations
std::vector<double> dimensionality_reduction::PCA::compute_mean(const mytrix::DenseMatrix& X) {
    const int rows = X.rows();
    const int cols = X.cols();
    std::vector<double> mean(static_cast<std::size_t>(cols), 0.0);
    if (rows == 0) return mean;
    for (int j = 0; j < cols; ++j) {
        mean[static_cast<std::size_t>(j)] = X.data.col(j).mean();
    }
    return mean;
}

std::vector<double> dimensionality_reduction::PCA::compute_std(const mytrix::DenseMatrix& X, const std::vector<double>& mean) {
    const int rows = X.rows();
    const int cols = X.cols();
    std::vector<double> std_dev(static_cast<std::size_t>(cols), 1.0);
    if (rows < 2) return std_dev;
    for (int j = 0; j < cols; ++j) {
        double sum_sq = 0.0;
        for (int i = 0; i < rows; ++i) {
            const double d = X.data(i, j) - mean[static_cast<std::size_t>(j)];
            sum_sq += d * d;
        }
        const double sd = std::sqrt(sum_sq / static_cast<double>(rows - 1));
        std_dev[static_cast<std::size_t>(j)] = (sd > 1e-12) ? sd : 1.0;
    }
    return std_dev;
}

mytrix::DenseMatrix dimensionality_reduction::PCA::preprocess(const mytrix::DenseMatrix& X) {
    return X;
}

void dimensionality_reduction::PCA::fit(const Matrix& X) {
    const int rows = X.rows();
    const int cols = X.cols();
    if (rows == 0 || cols == 0) {
        fitted_ = false;
        throw std::invalid_argument("PCA::fit requires a non-empty matrix");
    }

    mean_  = center_     ? compute_mean(X)        : std::vector<double>(static_cast<std::size_t>(cols), 0.0);
    scale_ = scale_data_ ? compute_std(X, mean_)  : std::vector<double>(static_cast<std::size_t>(cols), 1.0);

    Eigen::MatrixXd centered(rows, cols);
    for (int j = 0; j < cols; ++j) {
        for (int i = 0; i < rows; ++i) {
            centered(i, j) = (X.data(i, j) - mean_[static_cast<std::size_t>(j)]) / scale_[static_cast<std::size_t>(j)];
        }
    }

    svd_.compute(Matrix(centered));
    const auto all_singular_values = svd_.get_singular_values();

    const int max_components = std::min(rows, cols);
    int k = n_components_;
    if (k <= 0 || k > max_components) k = max_components;

    const Matrix V = svd_.get_V(); // cols x max_components (thin SVD)
    components_ = Matrix(V.data.leftCols(k).transpose().eval()); // k x cols

    const int dof = std::max(rows - 1, 1);
    double total_variance = 0.0;
    for (const double s : all_singular_values) total_variance += (s * s) / static_cast<double>(dof);

    explained_variance_.assign(static_cast<std::size_t>(k), 0.0);
    explained_variance_ratio_.assign(static_cast<std::size_t>(k), 0.0);
    singular_values_.assign(static_cast<std::size_t>(k), 0.0);
    for (int i = 0; i < k; ++i) {
        const double s = all_singular_values[static_cast<std::size_t>(i)];
        const double var = (s * s) / static_cast<double>(dof);
        singular_values_[static_cast<std::size_t>(i)] = s;
        explained_variance_[static_cast<std::size_t>(i)] = var;
        explained_variance_ratio_[static_cast<std::size_t>(i)] = (total_variance > 0.0) ? (var / total_variance) : 0.0;
    }

    fitted_ = true;
}

dimensionality_reduction::PCA::Matrix dimensionality_reduction::PCA::transform(const Matrix& X) const {
    if (!fitted_) throw std::runtime_error("PCA not fitted yet");
    const int rows = X.rows();
    const int cols = X.cols();
    Eigen::MatrixXd centered(rows, cols);
    for (int j = 0; j < cols; ++j) {
        for (int i = 0; i < rows; ++i) {
            centered(i, j) = (X.data(i, j) - mean_[static_cast<std::size_t>(j)]) / scale_[static_cast<std::size_t>(j)];
        }
    }
    return PCA::Matrix((centered * components_.data.transpose()).eval());
}

dimensionality_reduction::PCA::Matrix dimensionality_reduction::PCA::fit_transform(const Matrix& X) {
    fit(X);
    return transform(X);
}

dimensionality_reduction::PCA::Matrix dimensionality_reduction::PCA::inverse_transform(const Matrix& X_transformed) const {
    if (!fitted_) throw std::runtime_error("PCA not fitted yet");
    const Eigen::MatrixXd reconstructed_centered = X_transformed.data * components_.data;
    const int rows = static_cast<int>(reconstructed_centered.rows());
    const int cols = static_cast<int>(reconstructed_centered.cols());
    Eigen::MatrixXd result(rows, cols);
    for (int j = 0; j < cols; ++j) {
        for (int i = 0; i < rows; ++i) {
            result(i, j) = reconstructed_centered(i, j) * scale_[static_cast<std::size_t>(j)] + mean_[static_cast<std::size_t>(j)];
        }
    }
    return PCA::Matrix(result);
}

dimensionality_reduction::PCA::Matrix dimensionality_reduction::PCA::get_components() const {
    return components_;
}

dimensionality_reduction::PCA::Vector dimensionality_reduction::PCA::get_explained_variance() const {
    return explained_variance_;
}

dimensionality_reduction::PCA::Vector dimensionality_reduction::PCA::get_explained_variance_ratio() const {
    return explained_variance_ratio_;
}

dimensionality_reduction::PCA::Vector dimensionality_reduction::PCA::get_singular_values() const {
    return singular_values_;
}

