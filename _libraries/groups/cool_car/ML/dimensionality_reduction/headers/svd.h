
#ifndef SVD_H
#define SVD_H

#include "lib_metadata.h"
#include "mytrix_eigen_compat.hpp"
#include <stdexcept>
#include <vector>

namespace dimensionality_reduction {

class SVD {
public:
    using Matrix = matrix::DenseMatrix;
    using Vector = std::vector<double>;

    explicit SVD(bool compute_full_matrices = false);
    void compute(const Matrix& X);
    Matrix get_U() const;
    Vector get_singular_values() const;
    Matrix get_V() const;
    Matrix get_S() const;
    Matrix reconstruct(int num_components = 0) const;
    int rank(double tolerance = -1.0) const;
    Vector explained_variance_ratio() const;
    bool is_computed() const { return computed_; }

private:
    bool compute_full_matrices_;
    bool computed_;
    Matrix U_;
    Vector S_;
    Matrix V_;
};

} // namespace dimensionality_reduction

#endif // SVD_H
