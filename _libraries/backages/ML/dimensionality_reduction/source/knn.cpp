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

std::pair<std::vector<int>, std::vector<double>> KNN::find_neighbors_single(
    const Eigen::VectorXd& query, bool exclude_self, int self_index) const {
    
    int n = X_train_.rows();
    std::vector<std::pair<double, int>> distances;
    distances.reserve(n);
    
    for (int i = 0; i < n; ++i) {
        if (exclude_self && i == self_index) continue;
        double d = compute_distance(query, X_train_.row(i));
        distances.push_back({d, i});
    }
    
    int actual_k = std::min(k_, (int)distances.size());
    std::partial_sort(distances.begin(), distances.begin() + actual_k, distances.end());
    
    std::vector<int> indices(actual_k);
    std::vector<double> dists(actual_k);
    for (int i = 0; i < actual_k; ++i) {
        indices[i] = distances[i].second;
        dists[i] = distances[i].first;
    }
    return {indices, dists};
}

std::pair<Eigen::MatrixXi, Eigen::MatrixXd> KNN::kneighbors(const Eigen::MatrixXd& X_query) const {
    if (!fitted_) throw std::runtime_error("KNN not fitted yet");
    int n = X_query.rows();
    Eigen::MatrixXi indices(n, k_);
    Eigen::MatrixXd distances(n, k_);
    
    for (int i = 0; i < n; ++i) {
        auto [idx, dist] = find_neighbors_single(X_query.row(i));
        for (int j = 0; j < k_ && j < (int)idx.size(); ++j) {
            indices(i, j) = idx[j];
            distances(i, j) = dist[j];
        }
    }
    return {indices, distances};
}

std::pair<Eigen::MatrixXi, Eigen::MatrixXd> KNN::kneighbors() const {
    if (!fitted_) throw std::runtime_error("KNN not fitted yet");
    int n = X_train_.rows();
    Eigen::MatrixXi indices(n, k_);
    Eigen::MatrixXd distances(n, k_);
    
    for (int i = 0; i < n; ++i) {
        auto [idx, dist] = find_neighbors_single(X_train_.row(i), true, i);
        for (int j = 0; j < k_ && j < (int)idx.size(); ++j) {
            indices(i, j) = idx[j];
            distances(i, j) = dist[j];
        }
    }
    return {indices, distances};
}

Eigen::MatrixXd KNN::pairwise_distances(const Eigen::MatrixXd& X, const Eigen::MatrixXd& Y) const {
    int n = X.rows();
    int m = Y.rows();
    Eigen::MatrixXd D(n, m);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            D(i, j) = compute_distance(X.row(i), Y.row(j));
    return D;
}

} // namespace dimensionality_reduction
