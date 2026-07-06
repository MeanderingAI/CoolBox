#pragma once

#include "lib_metadata.h"
#include "mytrix_eigen_compat.hpp"
#include "priority_queue.h"      // data_structures::IndexedPriorityQueue
#include "van_emde_boas.h"       // data_structures::VanEmdeBoasTree
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace ml {
namespace kmeans {

/**
 * K-Means centroid initialisation strategies.
 */
enum class InitMethod {
    RANDOM,     ///< Uniform random selection from data points
    KMEANSPP    ///< K-Means++ weighted distance-squared sampling
};

/**
 * KMeans
 *
 * Lloyd's algorithm with optional K-Means++ initialisation.
 *
 * Internal data structures (from trekker::DATASTRUCTURE):
 *   - data_structures::IndexedPriorityQueue — used during K-Means++
 *     initialisation to maintain the minimum squared-distance to the
 *     current centroid set and draw the next centroid with probability
 *     proportional to that distance.  Priority decreases (min-heap)
 *     so the most-likely-to-be-chosen point is always accessible in O(1).
 *
 *   - data_structures::VanEmdeBoasTree — used in the assignment step to
 *     maintain the set of active cluster indices.  Successor/predecessor
 *     queries allow ordered traversal over the centroid set in O(log log k)
 *     time, enabling cache-friendly, branch-reduced assignment loops in
 *     quantised feature spaces.
 *
 * Complexity (n = points, d = dims, k = clusters, i = iterations):
 *   fit:     O(n·k·d·i)
 *   predict: O(n·k·d)
 */
class KMeans {
public:
    using Matrix = matrix::DenseMatrix;
    using Vector = std::vector<double>;

    /**
     * @param n_clusters    Number of clusters (k).
     * @param max_iter      Maximum Lloyd iterations.
     * @param init          Centroid initialisation strategy.
     * @param random_state  Seed for the internal PRNG.
     */
    explicit KMeans(int n_clusters  = 3,
                    int max_iter    = 300,
                    InitMethod init = InitMethod::KMEANSPP,
                    int random_state = 42);

    /** Fit the model to data matrix X (rows = samples, cols = features). */
    void fit(const Matrix& X);

    /** Assign each row of X to the nearest centroid. Requires fit(). */
    std::vector<int> predict(const Matrix& X) const;

    /** Fit and return cluster labels in one step. */
    std::vector<int> fit_predict(const Matrix& X);

    /** Return the k×d centroid matrix. */
    Matrix get_centroids() const;

    /** Return the cluster label for each training sample. */
    std::vector<int> get_labels() const;

    /** Return the final within-cluster sum of squared distances. */
    double get_inertia() const;

    /** Return per-iteration inertia for convergence diagnostics. */
    Vector get_inertia_history() const;

    /** Return the number of iterations run. */
    int get_n_iter() const;

    bool is_fitted() const { return fitted_; }

private:
    int        n_clusters_;
    int        max_iter_;
    InitMethod init_;
    int        random_state_;
    bool       fitted_ = false;

    Matrix           centroids_;
    std::vector<int> labels_;
    double           inertia_       = 0.0;
    Vector           inertia_history_;
    int              n_iter_        = 0;

    // --- initialisation helpers -----------------------------------------

    Matrix init_random(const Matrix& X, std::mt19937& rng) const;

    /**
     * K-Means++ initialisation.
     *
     * Uses data_structures::IndexedPriorityQueue<int,double,std::greater<double>>
     * (max-heap keyed by distance²) so the highest-priority (furthest) point
     * is always O(1) accessible.  After each centroid selection, priorities are
     * updated in O(log n) via decrease_key.
     */
    Matrix init_kmeanspp(const Matrix& X, std::mt19937& rng) const;

    // --- core EM steps --------------------------------------------------

    /**
     * Assignment step.
     *
     * Uses data_structures::VanEmdeBoasTree over [0, k) to iterate active
     * cluster indices in sorted order, enabling ordered-traversal assignment.
     */
    std::vector<int> assign(const Matrix& X, const Matrix& cents) const;

    Matrix update_centroids(const Matrix& X,
                            const std::vector<int>& labels,
                            int n_clusters) const;

    double calc_inertia(const Matrix& X,
                        const Matrix& cents,
                        const std::vector<int>& labels) const;

    static double sq_dist_row(const Matrix& X, int i,
                               const Matrix& C, int j);
};

} // namespace kmeans
} // namespace ml
