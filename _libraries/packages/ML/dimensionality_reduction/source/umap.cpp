


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
            random_state_(random_state), fitted_(false), X_train_(1, 1), embedding_(1, 1), a_(1.0), b_(1.0) {}

void UMAP::find_ab_params() {
    a_ = 1.93;
    b_ = 0.79;
    if (min_dist_ > 0.0) {
        b_ = 1.0 / (1.0 + min_dist_);
        a_ = 1.0;
    }
}

std::vector<double> UMAP::smooth_knn_dist(const matrix::DenseMatrix& distances, int k,
                                          int n_iter, double local_connectivity,
                                          double bandwidth) {
    return std::vector<double>(distances.rows(), 1.0);
}

std::pair<std::vector<std::vector<int>>, matrix::DenseMatrix> UMAP::compute_membership_strengths(
    const std::vector<std::vector<int>>& knn_indices, const matrix::DenseMatrix& knn_distances) {
    return {knn_indices, knn_distances};
}

matrix::DenseMatrix<double> UMAP::initialize_embedding(int n_samples) {
    return matrix::DenseMatrix(n_samples, n_components_);
}

double UMAP::compute_loss(const std::vector<double>& y_i, const std::vector<double>& y_j,
                          double weight) const {
    return 0.0;
}

std::vector<double> UMAP::compute_gradient(const std::vector<double>& y_i, const std::vector<double>& y_j,
                                           double weight, bool attractive) const {
    return std::vector<double>(y_i.size(), 0.0);
}

void UMAP::optimize_embedding(const matrix::DenseMatrix& graph_weights,
                               const std::vector<std::vector<int>>& graph_edges) {
    // TODO: Implement for DenseMatrix
}

void UMAP::fit(const matrix::DenseMatrix& X) {
    X_train_ = X;
    find_ab_params();
    embedding_ = initialize_embedding(X.rows());
    fitted_ = true;
}

matrix::DenseMatrix<double> UMAP::transform(const matrix::DenseMatrix<double>& X) const {
    if (!fitted_) throw std::runtime_error("UMAP not fitted yet");
    return matrix::DenseMatrix(X.rows(), n_components_);
}

matrix::DenseMatrix<double> UMAP::fit_transform(const matrix::DenseMatrix<double>& X) {
    fit(X);
    return embedding_;
}

} // namespace dimensionality_reduction
