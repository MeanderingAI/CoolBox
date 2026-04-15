#include "knn.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "lib_metadata.h"
LIBRARY_METADATA(knn, "KNN", "1.0.0", "k-Nearest Neighbors", "CoolBox");

namespace dimensionality_reduction {

KNN::KNN(int k, const std::string& metric)
    : k_(k), metric_(metric), fitted_(false) {}

void KNN::fit(const Eigen::MatrixXd& X) {
    X_train_ = X;
    fitted_ = true;
}

double KNN::compute_distance(const Eigen::VectorXd& x1, const Eigen::VectorXd& x2) const {
    if (metric_ == "euclidean") {
        return (x1 - x2).norm();
    } else if (metric_ == "manhattan") {
        return (x1 - x2).lpNorm<1>();
    } else if (metric_ == "cosine") {
        double dot = x1.dot(x2);
        double n1 = x1.norm();
        double n2 = x2.norm();
        if (n1 < 1e-10 || n2 < 1e-10) return 1.0;
        return 1.0 - dot / (n1 * n2);
    }
    return (x1 - x2).norm(); // default euclidean
}

std::pair<std::vector<Eigen::Index>, std::vector<double>> KNN::find_neighbors_single(
    const Eigen::VectorXd& query, bool exclude_self, Eigen::Index self_index) const {
    
    Eigen::Index n = X_train_.rows();
    std::vector<std::pair<double, Eigen::Index>> distances;
    distances.reserve(static_cast<size_t>(n));
    
    for (Eigen::Index i = 0; i < n; ++i) {
        if (exclude_self && i == self_index) continue;
        double d = compute_distance(query, X_train_.row(i));
        distances.push_back({d, i});
    }
    
    Eigen::Index actual_k = std::min(static_cast<Eigen::Index>(k_), static_cast<Eigen::Index>(distances.size()));
    std::partial_sort(distances.begin(), distances.begin() + actual_k, distances.end());
    
    std::vector<Eigen::Index> indices(static_cast<size_t>(actual_k));
    std::vector<double> dists(static_cast<size_t>(actual_k));
    for (Eigen::Index i = 0; i < actual_k; ++i) {
        indices[static_cast<size_t>(i)] = distances[static_cast<size_t>(i)].second;
        dists[static_cast<size_t>(i)] = distances[static_cast<size_t>(i)].first;
    }
    return {indices, dists};
}

std::pair<Eigen::MatrixXi, Eigen::MatrixXd> KNN::kneighbors(const Eigen::MatrixXd& X_query) const {
    if (!fitted_) throw std::runtime_error("KNN not fitted yet");
    Eigen::Index n = X_query.rows();
    Eigen::MatrixXi indices(n, k_);
    Eigen::MatrixXd distances(n, k_);
    
    for (Eigen::Index i = 0; i < n; ++i) {
        auto [idx, dist] = find_neighbors_single(X_query.row(i));
        for (Eigen::Index j = 0; j < static_cast<Eigen::Index>(k_) && j < static_cast<Eigen::Index>(idx.size()); ++j) {
            indices(static_cast<int>(i), static_cast<int>(j)) = static_cast<int>(idx[static_cast<size_t>(j)]);
            distances(static_cast<int>(i), static_cast<int>(j)) = dist[static_cast<size_t>(j)];
        }
    }
    return {indices, distances};
}

std::pair<Eigen::MatrixXi, Eigen::MatrixXd> KNN::kneighbors() const {
    if (!fitted_) throw std::runtime_error("KNN not fitted yet");
    Eigen::Index n = X_train_.rows();
    Eigen::MatrixXi indices(n, k_);
    Eigen::MatrixXd distances(n, k_);
    
    for (Eigen::Index i = 0; i < n; ++i) {
        auto [idx, dist] = find_neighbors_single(X_train_.row(i), true, i);
        for (Eigen::Index j = 0; j < static_cast<Eigen::Index>(k_) && j < static_cast<Eigen::Index>(idx.size()); ++j) {
            indices(static_cast<int>(i), static_cast<int>(j)) = static_cast<int>(idx[static_cast<size_t>(j)]);
            distances(static_cast<int>(i), static_cast<int>(j)) = dist[static_cast<size_t>(j)];
        }
    }
    return {indices, distances};
}

Eigen::MatrixXd KNN::pairwise_distances(const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y) const {
    Eigen::Index n = X.rows();
    Eigen::Index m = Y.rows();
    Eigen::MatrixXd D(static_cast<int>(n), static_cast<int>(m));
    for (Eigen::Index i = 0; i < n; ++i)
        for (Eigen::Index j = 0; j < m; ++j)
            D(static_cast<int>(i), static_cast<int>(j)) = compute_distance(X.row(i), Y.row(j));
    return D;
}

} // namespace dimensionality_reduction
