#include "lib_metadata.h"

LIBRARY_METADATA(
    knn,
    "KNN",
    "1.0",
    "K-Nearest Neighbors for classification and regression.",
    "ToolBox Authors"
)
#include "ML/dimensionality_reduction/knn.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace dimensionality_reduction {

KNN::KNN(int k, const std::string& metric)
    : k_(k), metric_(metric), fitted_(false) {
    
    if (k <= 0) {
        throw std::invalid_argument("k must be positive");
    }
    
    if (metric != "euclidean" && metric != "manhattan" && metric != "cosine") {
        throw std::invalid_argument("Unsupported metric: " + metric);
    }
}

void KNN::fit(const Eigen::MatrixXd& X) {
    if (X.rows() == 0 || X.cols() == 0) {
        throw std::runtime_error("Cannot fit KNN on empty data");
    }
    
    if (k_ >= X.rows()) {
        throw std::runtime_error("k must be less than number of samples");
    }
    
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
        double norm1 = x1.norm();
        double norm2 = x2.norm();
        
        if (norm1 == 0.0 || norm2 == 0.0) {
            return 1.0;  // Maximum cosine distance
        }
        
        // Cosine similarity to distance
        double similarity = dot / (norm1 * norm2);
        similarity = std::max(-1.0, std::min(1.0, similarity));  // Clamp to [-1, 1]
        return 1.0 - similarity;
    }
    
    return 0.0;  // Should never reach here
}

std::pair<std::vector<int>, std::vector<double>> KNN::find_neighbors_single(
    const Eigen::VectorXd& query, 
    bool exclude_self,
    int self_index) const {
    
    // Create a priority queue (max heap) to keep top k smallest distances
    auto cmp = [](const std::pair<double, int>& a, const std::pair<double, int>& b) {
        return a.first < b.first;  // Max heap based on distance
    };
    std::priority_queue<std::pair<double, int>, 
                       std::vector<std::pair<double, int>>, 
                       decltype(cmp)> pq(cmp);
    
    // Compute distances to all training points
    for (int i = 0; i < X_train_.rows(); ++i) {
        // Skip self if requested
        if (exclude_self && i == self_index) {
            continue;
        }
        
        double dist = compute_distance(query, X_train_.row(i));
        
        if (pq.size() < static_cast<size_t>(k_)) {
            pq.push({dist, i});
        } else if (dist < pq.top().first) {
            pq.pop();
            pq.push({dist, i});
        }
    }
    
    // Extract results (reverse order to get smallest first)
    std::vector<int> indices;
    std::vector<double> distances;
    
    while (!pq.empty()) {
        indices.push_back(pq.top().second);
        distances.push_back(pq.top().first);
        pq.pop();
    }
    
    // Reverse to get ascending order
    std::reverse(indices.begin(), indices.end());
    std::reverse(distances.begin(), distances.end());
    
    return {indices, distances};
}

std::pair<Eigen::MatrixXi, Eigen::MatrixXd> KNN::kneighbors(
    const Eigen::MatrixXd& X_query) const {
    
    if (!fitted_) {
        throw std::runtime_error("KNN not fitted. Call fit() first.");
    }
    
    if (X_query.cols() != X_train_.cols()) {
        throw std::runtime_error("Query data has different number of features");
    }
    
    int n_queries = X_query.rows();
    Eigen::MatrixXi indices(n_queries, k_);
    Eigen::MatrixXd distances(n_queries, k_);
    
    // Find neighbors for each query point
    for (int i = 0; i < n_queries; ++i) {
        auto [idx, dist] = find_neighbors_single(X_query.row(i), false, -1);
        
        for (int j = 0; j < k_; ++j) {
            indices(i, j) = idx[j];
            distances(i, j) = dist[j];
        }
    }
    
    return {indices, distances};
}

std::pair<Eigen::MatrixXi, Eigen::MatrixXd> KNN::kneighbors() const {
    if (!fitted_) {
        throw std::runtime_error("KNN not fitted. Call fit() first.");
    }
    
    int n_samples = X_train_.rows();
    Eigen::MatrixXi indices(n_samples, k_);
    Eigen::MatrixXd distances(n_samples, k_);
    
    // Find neighbors for each training point (excluding self)
    for (int i = 0; i < n_samples; ++i) {
        auto [idx, dist] = find_neighbors_single(X_train_.row(i), true, i);
        
        for (int j = 0; j < k_; ++j) {
            indices(i, j) = idx[j];
            distances(i, j) = dist[j];
        }
    }
    
    return {indices, distances};
}

Eigen::MatrixXd KNN::pairwise_distances(
    const Eigen::MatrixXd& X, 
    const Eigen::MatrixXd& Y) const {
    
    if (X.cols() != Y.cols()) {
        throw std::runtime_error("X and Y must have same number of features");
    }
    
    Eigen::MatrixXd distances(X.rows(), Y.rows());
    
    for (int i = 0; i < X.rows(); ++i) {
        for (int j = 0; j < Y.rows(); ++j) {
            distances(i, j) = compute_distance(X.row(i), Y.row(j));
        }
    }
    
    return distances;
}

} // namespace dimensionality_reduction
