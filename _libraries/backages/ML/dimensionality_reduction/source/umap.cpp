#include "umap.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

#include "lib_metadata.h"
LIBRARY_METADATA(umap, "UMAP", "1.0.0", "Uniform Manifold Approximation and Projection", "CoolBox");

namespace dimensionality_reduction {

UMAP::UMAP(int n_components, int n_neighbors, double min_dist,
           const std::string& metric, double learning_rate,
           int n_epochs, int random_state)
    : n_components_(n_components), n_neighbors_(n_neighbors), min_dist_(min_dist),
      metric_(metric), learning_rate_(learning_rate), n_epochs_(n_epochs),
      random_state_(random_state), fitted_(false), a_(1.0), b_(1.0) {}

void UMAP::find_ab_params() {
    // Approximate a, b parameters for the smooth approximation
    // For default min_dist=0.1, typical values are a≈1.93, b≈0.79
    a_ = 1.93;
    b_ = 0.79;
    if (min_dist_ > 0.0) {
        // Simple approximation
        b_ = 1.0 / (1.0 + min_dist_);
        a_ = 1.0;
    }
}

Eigen::VectorXd UMAP::smooth_knn_dist(const Eigen::MatrixXd& distances, int k,
                                        int n_iter, double local_connectivity,
                                        double bandwidth) {
    int n = distances.rows();
    Eigen::VectorXd sigmas(n);
    
    for (int i = 0; i < n; ++i) {
        double lo = 0.0, hi = 1000.0, mid = 1.0;
        double target = std::log2(static_cast<double>(k));
        
        // Find rho (distance to nearest neighbor)
        double rho = distances(i, 0);
        for (int j = 0; j < distances.cols(); ++j) {
            if (distances(i, j) > 0) { rho = distances(i, j); break; }
        }
        
        for (int iter = 0; iter < n_iter; ++iter) {
            mid = (lo + hi) / 2.0;
            double sum = 0.0;
            for (int j = 0; j < distances.cols(); ++j) {
                double d = std::max(distances(i, j) - rho, 0.0);
                sum += std::exp(-d / mid);
            }
            if (std::abs(sum - target) < 1e-5) break;
            if (sum > target) hi = mid;
            else lo = mid;
        }
        sigmas(i) = mid;
    }
    return sigmas;
}

std::pair<Eigen::MatrixXi, Eigen::MatrixXd> UMAP::compute_membership_strengths(
    const Eigen::MatrixXi& knn_indices, const Eigen::MatrixXd& knn_distances) {
    return {knn_indices, knn_distances};
}

Eigen::MatrixXd UMAP::initialize_embedding(int n_samples) {
    std::mt19937 gen(random_state_);
    std::normal_distribution<double> dist(0.0, 0.01);
    Eigen::MatrixXd embedding(n_samples, n_components_);
    for (int i = 0; i < n_samples; ++i)
        for (int j = 0; j < n_components_; ++j)
            embedding(i, j) = dist(gen);
    return embedding;
}

double UMAP::compute_loss(const Eigen::VectorXd& y_i, const Eigen::VectorXd& y_j,
                           double weight) const {
    double dist_sq = (y_i - y_j).squaredNorm();
    return -weight * std::log(1.0 / (1.0 + a_ * std::pow(dist_sq, b_)) + 1e-10);
}

Eigen::VectorXd UMAP::compute_gradient(const Eigen::VectorXd& y_i, const Eigen::VectorXd& y_j,
                                         double weight, bool attractive) const {
    Eigen::VectorXd diff = y_i - y_j;
    double dist_sq = diff.squaredNorm();
    double grad_coeff;
    if (attractive) {
        grad_coeff = -2.0 * a_ * b_ * std::pow(dist_sq, b_ - 1.0) /
                     (1.0 + a_ * std::pow(dist_sq, b_));
    } else {
        double d = 1.0 + a_ * std::pow(dist_sq, b_);
        grad_coeff = 2.0 * b_ / (dist_sq * d + 1e-6);
    }
    return grad_coeff * diff;
}

void UMAP::optimize_embedding(const Eigen::MatrixXd& graph_weights,
                               const Eigen::MatrixXi& graph_edges) {
    std::mt19937 gen(random_state_);
    int n = embedding_.rows();
    
    for (int epoch = 0; epoch < n_epochs_; ++epoch) {
        double alpha = learning_rate_ * (1.0 - static_cast<double>(epoch) / n_epochs_);
        
        for (int i = 0; i < graph_edges.rows(); ++i) {
            for (int j = 0; j < graph_edges.cols(); ++j) {
                int neighbor = graph_edges(i, j);
                if (neighbor < 0 || neighbor >= n) continue;
                
                Eigen::VectorXd grad = compute_gradient(
                    embedding_.row(i), embedding_.row(neighbor), 1.0, true);
                embedding_.row(i) -= alpha * grad.transpose();
                embedding_.row(neighbor) += alpha * grad.transpose();
            }
        }
    }
}

void UMAP::fit(const Eigen::MatrixXd& X) {
    X_train_ = X;
    find_ab_params();
    
    KNN knn(n_neighbors_, metric_);
    knn.fit(X);
    auto [indices, distances] = knn.kneighbors();
    
    embedding_ = initialize_embedding(X.rows());
    optimize_embedding(distances, indices);
    
    fitted_ = true;
}

Eigen::MatrixXd UMAP::transform(const Eigen::MatrixXd& X) const {
    if (!fitted_) throw std::runtime_error("UMAP not fitted yet");
    // Simplified: for new points, find nearest neighbors in training set
    // and interpolate embedding
    KNN knn(n_neighbors_, metric_);
    knn.fit(X_train_);
    auto [indices, distances] = knn.kneighbors(X);
    
    int n = X.rows();
    Eigen::MatrixXd result(n, n_components_);
    for (int i = 0; i < n; ++i) {
        Eigen::VectorXd weights(n_neighbors_);
        double total_weight = 0.0;
        for (int j = 0; j < n_neighbors_; ++j) {
            weights(j) = 1.0 / (distances(i, j) + 1e-10);
            total_weight += weights(j);
        }
        weights /= total_weight;
        
        result.row(i).setZero();
        for (int j = 0; j < n_neighbors_; ++j) {
            result.row(i) += weights(j) * embedding_.row(indices(i, j));
        }
    }
    return result;
}

Eigen::MatrixXd UMAP::fit_transform(const Eigen::MatrixXd& X) {
    fit(X);
    return embedding_;
}

Eigen::MatrixXd UMAP::get_embedding() const {
    if (!fitted_) throw std::runtime_error("UMAP not fitted yet");
    return embedding_;
}

} // namespace dimensionality_reduction
