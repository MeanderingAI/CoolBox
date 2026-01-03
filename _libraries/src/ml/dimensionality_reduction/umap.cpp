#include "lib_metadata.h"

LIBRARY_METADATA(
    umap,
    "UMAP",
    "1.0",
    "Uniform Manifold Approximation and Projection for nonlinear dimensionality reduction.",
    "ToolBox Authors"
)
#include "ml/dimensionality_reduction/umap.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>

namespace dimensionality_reduction {

UMAP::UMAP(
    int n_components,
    int n_neighbors,
    double min_dist,
    const std::string& metric,
    double learning_rate,
    int n_epochs,
    int random_state)
    : n_components_(n_components),
      n_neighbors_(n_neighbors),
      min_dist_(min_dist),
      metric_(metric),
      learning_rate_(learning_rate),
      n_epochs_(n_epochs),
      random_state_(random_state),
      fitted_(false),
      knn_(n_neighbors, metric),
      rng_(random_state) {
    
    if (n_components <= 0) {
        throw std::invalid_argument("n_components must be positive");
    }
    if (n_neighbors <= 0) {
        throw std::invalid_argument("n_neighbors must be positive");
    }
    if (min_dist < 0 || min_dist > 1) {
        throw std::invalid_argument("min_dist must be in [0, 1]");
    }
    
    find_ab_params();
}

void UMAP::find_ab_params() {
    // Approximate a and b from min_dist using curve fitting
    // These parameters control the low-dimensional attractive force
    if (min_dist_ == 0.0) {
        a_ = 1.0;
        b_ = 1.0;
    } else {
        a_ = 1.929 / (1.0 + 0.9 * std::pow(min_dist_, 1.25));
        b_ = std::pow(min_dist_, 0.7);
    }
}

Eigen::VectorXd UMAP::smooth_knn_dist(
    const Eigen::MatrixXd& distances,
    int k,
    int n_iter,
    double local_connectivity,
    double bandwidth) {
    
    int n_samples = distances.rows();
    Eigen::VectorXd sigmas = Eigen::VectorXd::Ones(n_samples);
    
    double target = std::log2(k) * bandwidth;
    
    for (int i = 0; i < n_samples; ++i) {
        double lo = 0.0;
        double hi = std::numeric_limits<double>::infinity();
        double mid = 1.0;
        
        // Binary search for sigma
        for (int iter = 0; iter < n_iter; ++iter) {
            double psum = 0.0;
            
            for (int j = 0; j < k; ++j) {
                double d = std::max(0.0, distances(i, j) - distances(i, 0));
                if (mid > 0.0) {
                    psum += std::exp(-d / mid);
                }
            }
            
            if (std::abs(psum - target) < 1e-5) {
                break;
            }
            
            if (psum > target) {
                hi = mid;
                mid = (lo + hi) / 2.0;
            } else {
                lo = mid;
                if (hi == std::numeric_limits<double>::infinity()) {
                    mid *= 2.0;
                } else {
                    mid = (lo + hi) / 2.0;
                }
            }
        }
        
        sigmas(i) = mid;
    }
    
    return sigmas;
}

std::pair<Eigen::MatrixXi, Eigen::MatrixXd> UMAP::compute_membership_strengths(
    const Eigen::MatrixXi& knn_indices,
    const Eigen::MatrixXd& knn_distances) {
    
    int n_samples = knn_indices.rows();
    int k = knn_indices.cols();
    
    // Compute smooth kNN distances (local sigma for each point)
    Eigen::VectorXd sigmas = smooth_knn_dist(knn_distances, k);
    
    // Build symmetric graph with membership strengths
    std::vector<std::pair<int, int>> edges;
    std::vector<double> weights;
    
    for (int i = 0; i < n_samples; ++i) {
        double rho = knn_distances(i, 0);  // Distance to nearest neighbor
        
        for (int j = 0; j < k; ++j) {
            int neighbor = knn_indices(i, j);
            double dist = knn_distances(i, j);
            
            if (neighbor == i) continue;  // Skip self
            
            // Compute membership strength
            double d_normalized = std::max(0.0, dist - rho);
            double strength = 0.0;
            
            if (sigmas(i) > 0.0) {
                strength = std::exp(-d_normalized / sigmas(i));
            }
            
            edges.push_back({i, neighbor});
            weights.push_back(strength);
        }
    }
    
    // Convert to matrices (store as COO format approximation)
    Eigen::MatrixXi edge_matrix(edges.size(), 2);
    Eigen::MatrixXd weight_matrix(edges.size(), 1);
    
    for (size_t i = 0; i < edges.size(); ++i) {
        edge_matrix(i, 0) = edges[i].first;
        edge_matrix(i, 1) = edges[i].second;
        weight_matrix(i, 0) = weights[i];
    }
    
    return std::make_pair(edge_matrix, weight_matrix);
}

Eigen::MatrixXd UMAP::initialize_embedding(int n_samples) {
    // Initialize with random embedding
    std::normal_distribution<double> dist(0.0, 1.0);
    
    Eigen::MatrixXd embedding(n_samples, n_components_);
    
    for (int i = 0; i < n_samples; ++i) {
        for (int j = 0; j < n_components_; ++j) {
            embedding(i, j) = dist(rng_) * 10.0 / n_components_;
        }
    }
    
    return embedding;
}

double UMAP::compute_loss(
    const Eigen::VectorXd& y_i,
    const Eigen::VectorXd& y_j,
    double weight) const {
    
    double dist_sq = (y_i - y_j).squaredNorm();
    
    // Attractive force (for positive edges)
    double attractive = -weight * std::log(1.0 + a_ * std::pow(dist_sq, b_));
    
    // Repulsive force (for negative samples)
    double repulsive = (1.0 - weight) * std::log(1.0 + a_ * std::pow(dist_sq, b_));
    
    return attractive + repulsive;
}

Eigen::VectorXd UMAP::compute_gradient(
    const Eigen::VectorXd& y_i,
    const Eigen::VectorXd& y_j,
    double weight,
    bool attractive) const {
    
    Eigen::VectorXd diff = y_i - y_j;
    double dist_sq = diff.squaredNorm();
    
    if (dist_sq < 1e-12) {
        return Eigen::VectorXd::Zero(n_components_);
    }
    
    double grad_coeff;
    
    if (attractive) {
        // Gradient of attractive force
        double denom = 1.0 + a_ * std::pow(dist_sq, b_);
        grad_coeff = -2.0 * a_ * b_ * weight * std::pow(dist_sq, b_ - 1.0) / denom;
    } else {
        // Gradient of repulsive force
        double denom = (dist_sq + 1.0) * (1.0 + a_ * std::pow(dist_sq, b_));
        grad_coeff = 2.0 * b_ / denom;
    }
    
    return grad_coeff * diff;
}

void UMAP::optimize_embedding(
    const Eigen::MatrixXd& graph_weights,
    const Eigen::MatrixXi& graph_edges) {
    
    int n_edges = graph_edges.rows();
    int n_samples = embedding_.rows();
    
    // Negative sampling rate
    int neg_sample_rate = 5;
    
    std::uniform_int_distribution<int> sample_dist(0, n_samples - 1);
    
    double alpha = learning_rate_;
    
    for (int epoch = 0; epoch < n_epochs_; ++epoch) {
        // Decay learning rate
        alpha = learning_rate_ * (1.0 - static_cast<double>(epoch) / n_epochs_);
        
        // Shuffle edges
        std::vector<int> edge_indices(n_edges);
        std::iota(edge_indices.begin(), edge_indices.end(), 0);
        std::shuffle(edge_indices.begin(), edge_indices.end(), rng_);
        
        for (int edge_idx : edge_indices) {
            int i = graph_edges(edge_idx, 0);
            int j = graph_edges(edge_idx, 1);
            double weight = graph_weights(edge_idx, 0);
            
            // Attractive force (positive edge)
            Eigen::VectorXd grad = compute_gradient(
                embedding_.row(i), embedding_.row(j), weight, true);
            
            embedding_.row(i) -= alpha * grad.transpose();
            
            // Negative sampling (repulsive force)
            for (int neg = 0; neg < neg_sample_rate; ++neg) {
                int k = sample_dist(rng_);
                if (k == i) continue;
                
                Eigen::VectorXd neg_grad = compute_gradient(
                    embedding_.row(i), embedding_.row(k), 0.0, false);
                
                embedding_.row(i) -= alpha * neg_grad.transpose();
            }
        }
        
        // Progress reporting
        if ((epoch + 1) % 50 == 0 || epoch == 0) {
            std::cout << "UMAP epoch " << (epoch + 1) << "/" << n_epochs_ << std::endl;
        }
    }
}

void UMAP::fit(const Eigen::MatrixXd& X) {
    if (X.rows() < n_neighbors_ + 1) {
        throw std::runtime_error("Need at least n_neighbors + 1 samples");
    }
    
    X_train_ = X;
    
    std::cout << "Computing nearest neighbors..." << std::endl;
    
    // Compute k-nearest neighbors
    knn_.fit(X);
    auto [knn_indices, knn_distances] = knn_.kneighbors();
    
    std::cout << "Computing fuzzy simplicial set..." << std::endl;
    
    // Compute membership strengths (high-dimensional graph)
    auto [edges, weights] = compute_membership_strengths(knn_indices, knn_distances);
    
    std::cout << "Initializing embedding..." << std::endl;
    
    // Initialize low-dimensional embedding
    embedding_ = initialize_embedding(X.rows());
    
    std::cout << "Optimizing embedding..." << std::endl;
    
    // Optimize embedding
    optimize_embedding(weights, edges);
    
    fitted_ = true;
    
    std::cout << "UMAP fitting complete!" << std::endl;
}

Eigen::MatrixXd UMAP::transform(const Eigen::MatrixXd& X) const {
    if (!fitted_) {
        throw std::runtime_error("UMAP not fitted. Call fit() first.");
    }
    
    // For now, we only support transform on training data
    // Full transform requires additional optimization
    if (X.rows() != X_train_.rows() || (X - X_train_).norm() > 1e-10) {
        throw std::runtime_error(
            "Transform on new data not yet implemented. "
            "Use fit_transform() for training data.");
    }
    
    return embedding_;
}

Eigen::MatrixXd UMAP::fit_transform(const Eigen::MatrixXd& X) {
    fit(X);
    return embedding_;
}

Eigen::MatrixXd UMAP::get_embedding() const {
    if (!fitted_) {
        throw std::runtime_error("UMAP not fitted. Call fit() first.");
    }
    return embedding_;
}

} // namespace dimensionality_reduction
