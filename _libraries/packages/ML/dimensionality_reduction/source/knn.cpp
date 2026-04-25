
#include "knn.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "lib_metadata.h"
LIBRARY_METADATA(knn, "KNN", "1.0.0", "k-Nearest Neighbors", "CoolBox");

namespace dimensionality_reduction {

KNN::KNN(int k, const std::string& metric)
    : k_(k), metric_(metric), fitted_(false), X_train_(1, 1) {}

void KNN::fit(const Matrix& X) {
    X_train_ = X;
    fitted_ = true;
}

double KNN::compute_distance(const Vector& x1, const Vector& x2) const {
    if (x1.size() != x2.size()) throw std::runtime_error("Vector size mismatch");
    if (metric_ == "euclidean") {
        double sum = 0.0;
        for (size_t i = 0; i < x1.size(); ++i) sum += (x1[i] - x2[i]) * (x1[i] - x2[i]);
        return std::sqrt(sum);
    } else if (metric_ == "manhattan") {
        double sum = 0.0;
        for (size_t i = 0; i < x1.size(); ++i) sum += std::abs(x1[i] - x2[i]);
        return sum;
    } else if (metric_ == "cosine") {
        double dot = 0.0, n1 = 0.0, n2 = 0.0;
        for (size_t i = 0; i < x1.size(); ++i) {
            dot += x1[i] * x2[i];
            n1 += x1[i] * x1[i];
            n2 += x2[i] * x2[i];
        }
        if (n1 < 1e-10 || n2 < 1e-10) return 1.0;
        return 1.0 - dot / (std::sqrt(n1) * std::sqrt(n2));
    }
    // Default: euclidean
    double sum = 0.0;
    for (size_t i = 0; i < x1.size(); ++i) sum += (x1[i] - x2[i]) * (x1[i] - x2[i]);
    return std::sqrt(sum);
}

std::pair<std::vector<size_t>, std::vector<double>> KNN::find_neighbors_single(const Vector& query, bool exclude_self, size_t self_index) const {
    // TODO: Implement for DenseMatrix X_train_
    return std::make_pair(std::vector<size_t>(), std::vector<double>());
}

std::pair<std::vector<size_t>, KNN::Matrix> KNN::kneighbors(const Matrix& X_query) const {
    // TODO: Implement for DenseMatrix
    return std::make_pair(std::vector<size_t>(), KNN::Matrix(1, 1));
}

std::pair<std::vector<size_t>, KNN::Matrix> KNN::kneighbors() const {
    // TODO: Implement for DenseMatrix
    return std::make_pair(std::vector<size_t>(), KNN::Matrix(1, 1));
}

KNN::Matrix KNN::pairwise_distances(const Matrix& X, const Matrix& Y) const {

    // TODO: Implement for DenseMatrix
    return KNN::Matrix(1, 1);
}

} // namespace dimensionality_reduction
