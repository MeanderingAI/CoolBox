
#ifndef KNN_H
#define KNN_H

#include "lib_metadata.h"
#include "mytrix_eigen_compat.hpp"
#include <vector>
#include <queue>
#include <functional>
#include <limits>

namespace dimensionality_reduction {

class KNN {
public:
    using Matrix = mytrix::DenseMatrix;
    using Vector = std::vector<double>;

    explicit KNN(int k = 5, const std::string& metric = "euclidean");
    void fit(const Matrix& X);
    std::pair<std::vector<size_t>, Matrix> kneighbors(const Matrix& X_query) const;
    std::pair<std::vector<size_t>, Matrix> kneighbors() const;
    Matrix pairwise_distances(const Matrix& X, const Matrix& Y) const;
    int get_k() const { return k_; }
    std::string get_metric() const { return metric_; }
    bool is_fitted() const { return fitted_; }

private:
    int k_;
    std::string metric_;
    bool fitted_;
    Matrix X_train_;
    double compute_distance(const Vector& x1, const Vector& x2) const;
    std::pair<std::vector<size_t>, std::vector<double>> find_neighbors_single(const Vector& query, bool exclude_self = false, size_t self_index = size_t(-1)) const;
};

} // namespace dimensionality_reduction

#endif // KNN_H
