
 #ifndef PCA_H
 #define PCA_H

 #include "lib_metadata.h"
 #include "mytrix_eigen_compat.hpp"
 #include "svd.h"
 #include <vector>

 namespace dimensionality_reduction {

class PCA {
public:
    using Matrix = matrix::DenseMatrix;
    using Vector = std::vector<double>;

    explicit PCA(int n_components = 0, bool center = true, bool scale = false);
    void fit(const Matrix& X);
    Matrix transform(const Matrix& X) const;
    Matrix fit_transform(const Matrix& X);
    Matrix inverse_transform(const Matrix& X_transformed) const;
    Matrix get_components() const;
    Vector get_explained_variance() const;
    Vector get_explained_variance_ratio() const;
    Vector get_singular_values() const;

private:
    int n_components_;
    bool center_;
    bool scale_data_;
    bool fitted_;
    Vector mean_;
    Vector scale_;
    Matrix components_;
    Vector explained_variance_;
    Vector explained_variance_ratio_;
    Vector singular_values_;
    SVD svd_;
    static std::vector<double> compute_mean(const matrix::DenseMatrix& X);
    static std::vector<double> compute_std(const matrix::DenseMatrix& X, const std::vector<double>& mean);
    static matrix::DenseMatrix preprocess(const matrix::DenseMatrix& X);
};

 } // namespace dimensionality_reduction

 #endif // PCA_H
// Removed duplicate/old Eigen-based code below. All Eigen references replaced with matrix::DenseMatrix and std::vector<double> above.
