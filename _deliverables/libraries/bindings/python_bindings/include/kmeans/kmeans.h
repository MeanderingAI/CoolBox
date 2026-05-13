#pragma once

// Binding-side header for ml::kmeans — compiled into the pybind11 ml_core extension.
//
// This is a header-only Eigen implementation that mirrors the API of the C++
// library at _deliverables/libraries/groups/cool_car/ML/kmeans/headers/kmeans.h.
// The original library uses data_structures::IndexedPriorityQueue and
// data_structures::VanEmdeBoasTree internally; this binding stub reproduces the
// same algorithm with pure STL/Eigen so pybind11 users get identical behaviour
// without linking the full trekker::DATASTRUCTURE suite.

#include <Eigen/Dense>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace ml {
namespace kmeans {

enum class InitMethod { RANDOM, KMEANSPP };

// ---------------------------------------------------------------------------
// KMeansResult — returned by fit(), accessible from Python as named attrs
// ---------------------------------------------------------------------------
struct KMeansResult {
    Eigen::MatrixXd          centroids;       ///< k × d centroid matrix
    std::vector<int>         labels;          ///< per-sample cluster index
    std::vector<double>      inertia_history; ///< SSE per iteration
    double                   inertia;         ///< final SSE
    int                      n_iter;          ///< iterations performed
};

// ---------------------------------------------------------------------------
// KMeans
// ---------------------------------------------------------------------------
class KMeans {
public:
    explicit KMeans(int n_clusters   = 3,
                    int max_iter     = 300,
                    InitMethod init  = InitMethod::KMEANSPP,
                    int random_state = 42)
        : n_clusters_(n_clusters)
        , max_iter_(max_iter)
        , init_(init)
        , random_state_(random_state)
    {
        if (n_clusters_ < 1)
            throw std::invalid_argument("n_clusters must be >= 1");
    }

    /** Fit to data matrix X (rows = samples, cols = features). */
    void fit(const Eigen::MatrixXd& X)
    {
        if (X.rows() < n_clusters_)
            throw std::invalid_argument("n_samples must be >= n_clusters");

        std::mt19937 rng(static_cast<unsigned>(random_state_));
        Eigen::MatrixXd cents = (init_ == InitMethod::KMEANSPP)
            ? init_kmeanspp(X, rng)
            : init_random(X, rng);

        result_.inertia_history.clear();
        int iter = 0;
        for (; iter < max_iter_; ++iter) {
            std::vector<int> new_labels = assign(X, cents);
            result_.inertia_history.push_back(calc_inertia(X, cents, new_labels));
            Eigen::MatrixXd new_cents   = update_centroids(X, new_labels);
            if (new_cents.isApprox(cents, 1e-9)) { cents = new_cents; ++iter; break; }
            cents = new_cents;
        }

        result_.labels          = assign(X, cents);
        result_.centroids       = cents;
        result_.inertia         = calc_inertia(X, cents, result_.labels);
        result_.n_iter          = iter;
        fitted_                 = true;
    }

    /** Predict cluster index for each row of X. */
    std::vector<int> predict(const Eigen::MatrixXd& X) const
    {
        check_fitted();
        return assign(X, result_.centroids);
    }

    std::vector<int> fit_predict(const Eigen::MatrixXd& X)
    {
        fit(X);
        return result_.labels;
    }

    Eigen::MatrixXd          get_centroids()       const { check_fitted(); return result_.centroids; }
    std::vector<int>         get_labels()          const { check_fitted(); return result_.labels; }
    double                   get_inertia()         const { check_fitted(); return result_.inertia; }
    std::vector<double>      get_inertia_history() const { check_fitted(); return result_.inertia_history; }
    int                      get_n_iter()          const { check_fitted(); return result_.n_iter; }
    bool                     is_fitted()           const { return fitted_; }

private:
    int        n_clusters_;
    int        max_iter_;
    InitMethod init_;
    int        random_state_;
    bool       fitted_ = false;
    KMeansResult result_;

    void check_fitted() const {
        if (!fitted_) throw std::runtime_error("KMeans: call fit() first");
    }

    // ------------------------------------------------------------------
    // K-Means++ initialisation
    //
    // Mirrors the C++ library's use of IndexedPriorityQueue:
    //   priority[i] = min squared-distance from point i to nearest centroid
    // The next centroid is sampled proportional to that priority.
    // ------------------------------------------------------------------
    Eigen::MatrixXd init_kmeanspp(const Eigen::MatrixXd& X, std::mt19937& rng) const
    {
        const int n = static_cast<int>(X.rows());
        std::uniform_int_distribution<int> uni(0, n - 1);
        Eigen::MatrixXd cents(n_clusters_, X.cols());
        cents.row(0) = X.row(uni(rng));

        // IndexedPriorityQueue equivalent: vector of min-dist² per point
        std::vector<double> dist2(n, std::numeric_limits<double>::infinity());

        for (int c = 1; c < n_clusters_; ++c) {
            // Update min distances (priority-decrease equivalent)
            for (int i = 0; i < n; ++i) {
                double d = (X.row(i) - cents.row(c - 1)).squaredNorm();
                if (d < dist2[i]) dist2[i] = d;
            }
            // Weighted random draw
            double total = std::accumulate(dist2.begin(), dist2.end(), 0.0);
            std::uniform_real_distribution<double> ud(0.0, total);
            double r = ud(rng);
            int chosen = n - 1;
            for (int i = 0; i < n; ++i) {
                r -= dist2[i];
                if (r <= 0.0) { chosen = i; break; }
            }
            cents.row(c) = X.row(chosen);
        }
        return cents;
    }

    Eigen::MatrixXd init_random(const Eigen::MatrixXd& X, std::mt19937& rng) const
    {
        std::vector<int> idx(X.rows());
        std::iota(idx.begin(), idx.end(), 0);
        std::shuffle(idx.begin(), idx.end(), rng);
        Eigen::MatrixXd cents(n_clusters_, X.cols());
        for (int c = 0; c < n_clusters_; ++c)
            cents.row(c) = X.row(idx[c]);
        return cents;
    }

    // ------------------------------------------------------------------
    // Assignment step
    //
    // Mirrors the C++ library's use of VanEmdeBoasTree over [0, k) for
    // ordered traversal of active centroid indices.
    // ------------------------------------------------------------------
    std::vector<int> assign(const Eigen::MatrixXd& X,
                             const Eigen::MatrixXd& cents) const
    {
        const int n = static_cast<int>(X.rows());
        const int k = static_cast<int>(cents.rows());
        std::vector<int> labels(n);
        for (int i = 0; i < n; ++i) {
            double best = std::numeric_limits<double>::infinity();
            int    bi   = 0;
            // VanEmdeBoasTree successor traversal equivalent (k is small)
            for (int c = 0; c < k; ++c) {
                double d = (X.row(i) - cents.row(c)).squaredNorm();
                if (d < best) { best = d; bi = c; }
            }
            labels[i] = bi;
        }
        return labels;
    }

    Eigen::MatrixXd update_centroids(const Eigen::MatrixXd& X,
                                      const std::vector<int>& labels) const
    {
        Eigen::MatrixXd cents = Eigen::MatrixXd::Zero(n_clusters_, X.cols());
        std::vector<int> cnt(n_clusters_, 0);
        for (int i = 0; i < static_cast<int>(X.rows()); ++i) {
            cents.row(labels[i]) += X.row(i);
            cnt[labels[i]]++;
        }
        for (int c = 0; c < n_clusters_; ++c)
            if (cnt[c] > 0) cents.row(c) /= cnt[c];
        return cents;
    }

    double calc_inertia(const Eigen::MatrixXd& X,
                         const Eigen::MatrixXd& cents,
                         const std::vector<int>& labels) const
    {
        double sse = 0.0;
        for (int i = 0; i < static_cast<int>(X.rows()); ++i)
            sse += (X.row(i) - cents.row(labels[i])).squaredNorm();
        return sse;
    }
};

} // namespace kmeans
} // namespace ml
