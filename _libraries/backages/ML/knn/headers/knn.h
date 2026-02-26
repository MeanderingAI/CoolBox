/**
 * @file knn.h
 * @brief K-Nearest Neighbours classifier and regressor (header-only).
 *
 * Supports classification (majority vote) and regression (weighted/uniform average).
 * Distance metrics: Euclidean, Manhattan, Minkowski, Chebyshev.
 * Optional distance-weighted voting.
 *
 * Usage:
 * @code{.cpp}
 * using namespace ml;
 * KNN<double> knn(3, DistanceMetric::EUCLIDEAN, TaskType::CLASSIFICATION);
 *
 * Eigen::MatrixXd X_train(4, 2);
 * X_train << 1, 2,  3, 4,  5, 6,  7, 8;
 * Eigen::VectorXd y_train(4);
 * y_train << 0, 0, 1, 1;
 *
 * knn.fit(X_train, y_train);
 *
 * Eigen::MatrixXd X_test(1, 2);
 * X_test << 4, 5;
 * Eigen::VectorXd predictions = knn.predict(X_test);
 * @endcode
 */
#pragma once

#include <Eigen/Dense>
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <numeric>
#include <unordered_map>
#include <limits>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace ml {

// ===================================================================
// Enumerations
// ===================================================================

/**
 * @brief Distance metric used for neighbour computation.
 */
enum class DistanceMetric {
    EUCLIDEAN,   ///< L2 norm
    MANHATTAN,   ///< L1 norm
    MINKOWSKI,   ///< Lp norm (requires setting p)
    CHEBYSHEV    ///< L∞ norm
};

/**
 * @brief Task type: classification or regression.
 */
enum class TaskType {
    CLASSIFICATION,  ///< Majority vote among neighbours
    REGRESSION       ///< Mean (or weighted mean) of neighbour targets
};

/**
 * @brief Weighting scheme for neighbour contributions.
 */
enum class WeightType {
    UNIFORM,   ///< All neighbours contribute equally
    DISTANCE   ///< Closer neighbours contribute more (1/d)
};

// ===================================================================
// Neighbour result
// ===================================================================

/**
 * @brief Stores a single neighbour's index and distance.
 */
struct Neighbour {
    int index;       ///< Index in the training set
    double distance; ///< Distance to the query point
};

// ===================================================================
// KNN class
// ===================================================================

/**
 * @brief K-Nearest Neighbours classifier / regressor.
 *
 * @tparam Scalar  Floating-point type (default double).
 */
template <typename Scalar = double>
class KNN {
public:
    using VectorT = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;
    using MatrixT = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;

    // ---------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------

    /**
     * @brief Construct a KNN model.
     *
     * @param k              Number of neighbours (must be >= 1).
     * @param metric         Distance metric to use.
     * @param task           Classification or regression.
     * @param weight         Uniform or distance-weighted voting.
     * @param minkowski_p    The p parameter for Minkowski distance (must be >= 1).
     */
    explicit KNN(int k = 5,
                 DistanceMetric metric = DistanceMetric::EUCLIDEAN,
                 TaskType task = TaskType::CLASSIFICATION,
                 WeightType weight = WeightType::UNIFORM,
                 Scalar minkowski_p = 2.0)
        : k_(k)
        , metric_(metric)
        , task_(task)
        , weight_(weight)
        , minkowski_p_(minkowski_p)
        , fitted_(false)
    {
        if (k_ < 1) {
            throw std::invalid_argument("KNN: k must be >= 1, got " + std::to_string(k_));
        }
        if (minkowski_p_ < 1.0) {
            throw std::invalid_argument("KNN: Minkowski p must be >= 1.0");
        }
    }

    // ---------------------------------------------------------------
    // Fit
    // ---------------------------------------------------------------

    /**
     * @brief Store training data. KNN is lazy — no model is built.
     *
     * @param X  Training features (n_samples × n_features).
     * @param y  Training targets/labels (n_samples).
     */
    void fit(const MatrixT& X, const VectorT& y) {
        if (X.rows() != y.size()) {
            throw std::invalid_argument(
                "KNN::fit: X.rows() (" + std::to_string(X.rows()) +
                ") != y.size() (" + std::to_string(y.size()) + ")");
        }
        if (X.rows() == 0) {
            throw std::invalid_argument("KNN::fit: empty training set");
        }
        X_train_ = X;
        y_train_ = y;
        fitted_ = true;
    }

    // ---------------------------------------------------------------
    // Predict
    // ---------------------------------------------------------------

    /**
     * @brief Predict targets for one or more query points.
     *
     * @param X_query  Query features (n_queries × n_features).
     * @return VectorT Predicted labels/values (n_queries).
     */
    VectorT predict(const MatrixT& X_query) const {
        check_fitted();
        if (X_query.cols() != X_train_.cols()) {
            throw std::invalid_argument(
                "KNN::predict: feature dimension mismatch ("
                + std::to_string(X_query.cols()) + " vs "
                + std::to_string(X_train_.cols()) + ")");
        }

        const int n_queries = static_cast<int>(X_query.rows());
        VectorT predictions(n_queries);

        for (int i = 0; i < n_queries; ++i) {
            auto neighbours = find_neighbours(X_query.row(i));
            if (task_ == TaskType::CLASSIFICATION) {
                predictions(i) = classify(neighbours);
            } else {
                predictions(i) = regress(neighbours);
            }
        }
        return predictions;
    }

    /**
     * @brief Predict a single query point.
     *
     * @param query  Feature vector (n_features).
     * @return Scalar Predicted label/value.
     */
    Scalar predict_one(const VectorT& query) const {
        check_fitted();
        if (query.size() != X_train_.cols()) {
            throw std::invalid_argument("KNN::predict_one: feature dimension mismatch");
        }
        auto neighbours = find_neighbours(query.transpose());
        if (task_ == TaskType::CLASSIFICATION) {
            return classify(neighbours);
        }
        return regress(neighbours);
    }

    // ---------------------------------------------------------------
    // K-neighbours query
    // ---------------------------------------------------------------

    /**
     * @brief Return the k nearest neighbours for a query point.
     *
     * @param query  Feature vector (n_features).
     * @return std::vector<Neighbour> Sorted by ascending distance.
     */
    std::vector<Neighbour> kneighbours(const VectorT& query) const {
        check_fitted();
        return find_neighbours(query.transpose());
    }

    // ---------------------------------------------------------------
    // Score (accuracy / R²)
    // ---------------------------------------------------------------

    /**
     * @brief Compute accuracy (classification) or R² score (regression).
     *
     * @param X  Test features (n_samples × n_features).
     * @param y  True targets/labels (n_samples).
     * @return Scalar  Accuracy in [0,1] or R² score.
     */
    Scalar score(const MatrixT& X, const VectorT& y) const {
        VectorT preds = predict(X);
        if (task_ == TaskType::CLASSIFICATION) {
            int correct = 0;
            for (int i = 0; i < y.size(); ++i) {
                if (std::abs(preds(i) - y(i)) < static_cast<Scalar>(0.5)) {
                    ++correct;
                }
            }
            return static_cast<Scalar>(correct) / static_cast<Scalar>(y.size());
        } else {
            // R² = 1 - SS_res / SS_tot
            Scalar mean_y = y.mean();
            Scalar ss_res = (y - preds).squaredNorm();
            Scalar ss_tot = (y.array() - mean_y).matrix().squaredNorm();
            if (ss_tot < std::numeric_limits<Scalar>::epsilon()) {
                return (ss_res < std::numeric_limits<Scalar>::epsilon())
                    ? static_cast<Scalar>(1.0)
                    : static_cast<Scalar>(0.0);
            }
            return static_cast<Scalar>(1.0) - ss_res / ss_tot;
        }
    }

    // ---------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------

    int k() const { return k_; }
    void set_k(int k) {
        if (k < 1) throw std::invalid_argument("KNN: k must be >= 1");
        k_ = k;
    }

    DistanceMetric metric() const { return metric_; }
    void set_metric(DistanceMetric m) { metric_ = m; }

    TaskType task() const { return task_; }
    void set_task(TaskType t) { task_ = t; }

    WeightType weight() const { return weight_; }
    void set_weight(WeightType w) { weight_ = w; }

    Scalar minkowski_p() const { return minkowski_p_; }
    void set_minkowski_p(Scalar p) {
        if (p < 1.0) throw std::invalid_argument("KNN: Minkowski p must be >= 1.0");
        minkowski_p_ = p;
    }

    bool is_fitted() const { return fitted_; }
    int n_samples() const { return fitted_ ? static_cast<int>(X_train_.rows()) : 0; }
    int n_features() const { return fitted_ ? static_cast<int>(X_train_.cols()) : 0; }

    // ---------------------------------------------------------------
    // Summary
    // ---------------------------------------------------------------

    /**
     * @brief Return a human-readable summary string.
     */
    std::string summary() const {
        std::ostringstream oss;
        oss << "KNN(k=" << k_
            << ", metric=" << metric_name()
            << ", task=" << (task_ == TaskType::CLASSIFICATION ? "classification" : "regression")
            << ", weight=" << (weight_ == WeightType::UNIFORM ? "uniform" : "distance");
        if (metric_ == DistanceMetric::MINKOWSKI) {
            oss << ", p=" << minkowski_p_;
        }
        oss << ", fitted=" << (fitted_ ? "true" : "false");
        if (fitted_) {
            oss << ", n_samples=" << X_train_.rows()
                << ", n_features=" << X_train_.cols();
        }
        oss << ")";
        return oss.str();
    }

private:
    int k_;
    DistanceMetric metric_;
    TaskType task_;
    WeightType weight_;
    Scalar minkowski_p_;
    bool fitted_;

    MatrixT X_train_;
    VectorT y_train_;

    // ---------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------

    void check_fitted() const {
        if (!fitted_) {
            throw std::logic_error("KNN: model has not been fitted. Call fit() first.");
        }
    }

    /**
     * @brief Compute distance between two row vectors.
     */
    Scalar compute_distance(
        const Eigen::Block<const MatrixT, 1, Eigen::Dynamic, false>& a,
        const Eigen::Block<const MatrixT, 1, Eigen::Dynamic, false>& b) const
    {
        switch (metric_) {
            case DistanceMetric::EUCLIDEAN:
                return std::sqrt((a - b).squaredNorm());

            case DistanceMetric::MANHATTAN:
                return (a - b).cwiseAbs().sum();

            case DistanceMetric::MINKOWSKI: {
                Scalar sum = 0;
                for (int j = 0; j < a.size(); ++j) {
                    sum += std::pow(std::abs(a(j) - b(j)), minkowski_p_);
                }
                return std::pow(sum, static_cast<Scalar>(1.0) / minkowski_p_);
            }

            case DistanceMetric::CHEBYSHEV:
                return (a - b).cwiseAbs().maxCoeff();

            default:
                return std::sqrt((a - b).squaredNorm());
        }
    }

    /**
     * @brief Overload for VectorT-like row expression (from predict_one via transpose).
     */
    template <typename Derived>
    Scalar compute_distance_generic(
        const Eigen::MatrixBase<Derived>& a,
        const Eigen::Block<const MatrixT, 1, Eigen::Dynamic, false>& b) const
    {
        switch (metric_) {
            case DistanceMetric::EUCLIDEAN:
                return std::sqrt((a - b).squaredNorm());

            case DistanceMetric::MANHATTAN:
                return (a - b).cwiseAbs().sum();

            case DistanceMetric::MINKOWSKI: {
                Scalar sum = 0;
                for (int j = 0; j < a.size(); ++j) {
                    sum += std::pow(std::abs(a(j) - b(j)), minkowski_p_);
                }
                return std::pow(sum, static_cast<Scalar>(1.0) / minkowski_p_);
            }

            case DistanceMetric::CHEBYSHEV:
                return (a - b).cwiseAbs().maxCoeff();

            default:
                return std::sqrt((a - b).squaredNorm());
        }
    }

    /**
     * @brief Find the k nearest neighbours of a query row.
     */
    template <typename Derived>
    std::vector<Neighbour> find_neighbours(const Eigen::MatrixBase<Derived>& query_row) const {
        const int n = static_cast<int>(X_train_.rows());
        const int effective_k = std::min(k_, n);

        std::vector<Neighbour> all(n);
        for (int i = 0; i < n; ++i) {
            Scalar d = compute_distance_generic(query_row, X_train_.row(i));
            all[i] = {i, static_cast<double>(d)};
        }

        // Partial sort to get the k smallest
        std::partial_sort(all.begin(), all.begin() + effective_k, all.end(),
            [](const Neighbour& a, const Neighbour& b) {
                return a.distance < b.distance;
            });

        return std::vector<Neighbour>(all.begin(), all.begin() + effective_k);
    }

    /**
     * @brief Classification via (weighted) majority vote.
     */
    Scalar classify(const std::vector<Neighbour>& neighbours) const {
        // Use a map from label -> accumulated weight
        std::unordered_map<int, Scalar> votes;
        for (const auto& nb : neighbours) {
            int label = static_cast<int>(std::round(y_train_(nb.index)));
            Scalar w = compute_weight(nb.distance);
            votes[label] += w;
        }

        // Find the label with the highest vote
        int best_label = 0;
        Scalar best_weight = -1;
        for (const auto& [label, w] : votes) {
            if (w > best_weight) {
                best_weight = w;
                best_label = label;
            }
        }
        return static_cast<Scalar>(best_label);
    }

    /**
     * @brief Regression via (weighted) mean.
     */
    Scalar regress(const std::vector<Neighbour>& neighbours) const {
        Scalar weighted_sum = 0;
        Scalar weight_total = 0;
        for (const auto& nb : neighbours) {
            Scalar w = compute_weight(nb.distance);
            weighted_sum += w * y_train_(nb.index);
            weight_total += w;
        }
        if (weight_total < std::numeric_limits<Scalar>::epsilon()) {
            // All neighbours are at distance 0 — simple mean
            Scalar sum = 0;
            for (const auto& nb : neighbours) {
                sum += y_train_(nb.index);
            }
            return sum / static_cast<Scalar>(neighbours.size());
        }
        return weighted_sum / weight_total;
    }

    /**
     * @brief Compute the weight for a neighbour given its distance.
     */
    Scalar compute_weight(double distance) const {
        if (weight_ == WeightType::UNIFORM) {
            return static_cast<Scalar>(1.0);
        }
        // Distance weighting: 1/d (with epsilon to avoid division by zero)
        constexpr double eps = 1e-10;
        return static_cast<Scalar>(1.0 / (distance + eps));
    }

    /**
     * @brief Return the metric name as a string.
     */
    std::string metric_name() const {
        switch (metric_) {
            case DistanceMetric::EUCLIDEAN: return "euclidean";
            case DistanceMetric::MANHATTAN: return "manhattan";
            case DistanceMetric::MINKOWSKI: return "minkowski";
            case DistanceMetric::CHEBYSHEV: return "chebyshev";
            default: return "unknown";
        }
    }
};

} // namespace ml
