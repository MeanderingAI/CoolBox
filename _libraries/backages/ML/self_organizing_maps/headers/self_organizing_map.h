#pragma once

#include <Eigen/Dense>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>

/**
 * @file self_organizing_map.h
 * @brief Self-Organizing Map (Kohonen Map) implementation.
 *
 * A Self-Organizing Map is an unsupervised learning algorithm that
 * produces a low-dimensional (typically 2D) discretised representation
 * of the input space, preserving topological structure.
 *
 * Key features:
 *   - Configurable grid dimensions (rows × cols)
 *   - Gaussian or bubble neighbourhood function
 *   - Exponential decay for learning rate and neighbourhood radius
 *   - Best Matching Unit (BMU) lookup
 *   - U-Matrix computation for cluster boundary visualisation
 *   - Quantisation error metric
 */

namespace ml {

/**
 * @brief Neighbourhood function type.
 */
enum class NeighbourhoodType {
    GAUSSIAN,   ///< Smooth Gaussian falloff
    BUBBLE      ///< Binary: 1 if within radius, 0 otherwise
};

/**
 * @brief Initialisation strategy for weight vectors.
 */
enum class InitStrategy {
    RANDOM_UNIFORM, ///< Uniform random in [min, max] of each feature
    RANDOM_SAMPLE,  ///< Random samples from the training data
    PCA             ///< Span the first two principal components
};

/**
 * @brief Result of finding the Best Matching Unit.
 */
struct BMUResult {
    int row;        ///< Grid row index
    int col;        ///< Grid column index
    int flat_index; ///< row * cols + col
    double distance;///< Euclidean distance to BMU weight
};

/**
 * @brief Self-Organizing Map (Kohonen Map).
 *
 * @tparam Scalar  Floating-point type (default double).
 */
template <typename Scalar = double>
class SelfOrganizingMap {
public:
    using VectorT = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;
    using MatrixT = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;

private:
    int rows_;
    int cols_;
    int input_dim_;

    // Weight matrix: (rows_ * cols_) × input_dim_
    // Row i*cols_+j holds the weight vector for grid cell (i, j).
    MatrixT weights_;

    // Training hyperparameters
    Scalar initial_learning_rate_;
    Scalar initial_radius_;
    Scalar time_constant_;  // τ for exponential decay

    NeighbourhoodType neighbourhood_type_;

    std::mt19937 rng_;

public:
    // ---------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------

    /**
     * @brief Construct a SOM.
     * @param rows       Grid height.
     * @param cols       Grid width.
     * @param input_dim  Dimensionality of input vectors.
     * @param learning_rate  Initial learning rate (α₀).
     * @param radius     Initial neighbourhood radius (σ₀). Default: max(rows,cols)/2.
     * @param neighbourhood  Neighbourhood function type.
     * @param seed       RNG seed.
     */
    SelfOrganizingMap(int rows, int cols, int input_dim,
                      Scalar learning_rate = 0.5,
                      Scalar radius = -1,
                      NeighbourhoodType neighbourhood = NeighbourhoodType::GAUSSIAN,
                      unsigned seed = 42)
        : rows_(rows), cols_(cols), input_dim_(input_dim),
          initial_learning_rate_(learning_rate),
          neighbourhood_type_(neighbourhood),
          rng_(seed)
    {
        if (rows < 1 || cols < 1 || input_dim < 1)
            throw std::invalid_argument("SOM dimensions must be >= 1");

        initial_radius_ = (radius > 0) ? radius
                          : static_cast<Scalar>(std::max(rows, cols)) / 2.0;
        time_constant_ = 1.0;  // set during fit()

        weights_ = MatrixT::Zero(rows_ * cols_, input_dim_);
    }

    // ---------------------------------------------------------------
    // Initialisation
    // ---------------------------------------------------------------

    /**
     * @brief Initialise weights from data.
     */
    void initialize(const MatrixT& data,
                    InitStrategy strategy = InitStrategy::RANDOM_UNIFORM) {
        if (data.cols() != input_dim_)
            throw std::invalid_argument("Data dimension mismatch");

        switch (strategy) {
        case InitStrategy::RANDOM_UNIFORM:
            init_random_uniform(data);
            break;
        case InitStrategy::RANDOM_SAMPLE:
            init_random_sample(data);
            break;
        case InitStrategy::PCA:
            init_pca(data);
            break;
        }
    }

    // ---------------------------------------------------------------
    // Training
    // ---------------------------------------------------------------

    /**
     * @brief Train the SOM on data for a number of epochs.
     * @param data   N × input_dim matrix (one sample per row).
     * @param epochs Number of training epochs.
     */
    void fit(const MatrixT& data, int epochs = 100) {
        if (data.cols() != input_dim_)
            throw std::invalid_argument("Data dimension mismatch");

        int n_samples = static_cast<int>(data.rows());
        int total_iterations = epochs * n_samples;
        time_constant_ = static_cast<Scalar>(total_iterations)
                       / std::log(initial_radius_ + 1);

        // Indices for shuffling
        std::vector<int> indices(n_samples);
        std::iota(indices.begin(), indices.end(), 0);

        int iteration = 0;
        for (int epoch = 0; epoch < epochs; ++epoch) {
            std::shuffle(indices.begin(), indices.end(), rng_);

            for (int idx : indices) {
                VectorT sample = data.row(idx).transpose();

                // 1. Find BMU
                BMUResult bmu = find_bmu(sample);

                // 2. Compute decayed parameters
                Scalar lr = learning_rate(iteration, total_iterations);
                Scalar radius = neighbourhood_radius(iteration, total_iterations);

                // 3. Update weights
                update_weights(sample, bmu, lr, radius);

                ++iteration;
            }
        }
    }

    /**
     * @brief Single-step online update with one sample.
     */
    void partial_fit(const VectorT& sample, int iteration, int total_iterations) {
        BMUResult bmu = find_bmu(sample);
        Scalar lr = learning_rate(iteration, total_iterations);
        Scalar radius = neighbourhood_radius(iteration, total_iterations);
        update_weights(sample, bmu, lr, radius);
    }

    // ---------------------------------------------------------------
    // Inference
    // ---------------------------------------------------------------

    /**
     * @brief Find the Best Matching Unit for a sample.
     */
    BMUResult find_bmu(const VectorT& sample) const {
        BMUResult result{0, 0, 0, std::numeric_limits<Scalar>::max()};

        for (int i = 0; i < rows_ * cols_; ++i) {
            Scalar dist = (weights_.row(i).transpose() - sample).squaredNorm();
            if (dist < result.distance) {
                result.flat_index = i;
                result.row = i / cols_;
                result.col = i % cols_;
                result.distance = dist;
            }
        }
        result.distance = std::sqrt(result.distance);
        return result;
    }

    /**
     * @brief Map each data row to its BMU grid coordinate.
     * @return N × 2 matrix of (row, col) assignments.
     */
    Eigen::MatrixXi transform(const MatrixT& data) const {
        Eigen::MatrixXi assignments(data.rows(), 2);
        for (int i = 0; i < data.rows(); ++i) {
            VectorT sample = data.row(i).transpose();
            BMUResult bmu = find_bmu(sample);
            assignments(i, 0) = bmu.row;
            assignments(i, 1) = bmu.col;
        }
        return assignments;
    }

    // ---------------------------------------------------------------
    // Metrics
    // ---------------------------------------------------------------

    /**
     * @brief Mean quantisation error: average distance from each sample
     *        to its BMU.
     */
    Scalar quantisation_error(const MatrixT& data) const {
        Scalar total = 0;
        for (int i = 0; i < data.rows(); ++i) {
            VectorT sample = data.row(i).transpose();
            BMUResult bmu = find_bmu(sample);
            total += bmu.distance;
        }
        return total / static_cast<Scalar>(data.rows());
    }

    /**
     * @brief Compute the U-Matrix (unified distance matrix).
     *
     * For each neuron, the U-Matrix value is the mean distance to its
     * direct neighbours. High values indicate cluster boundaries.
     *
     * @return rows_ × cols_ matrix of mean neighbour distances.
     */
    MatrixT u_matrix() const {
        MatrixT umat = MatrixT::Zero(rows_, cols_);

        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                VectorT w = weights_.row(r * cols_ + c).transpose();
                Scalar sum = 0;
                int count = 0;

                // Check 4-connected neighbours
                int dr[] = {-1, 1, 0, 0};
                int dc[] = {0, 0, -1, 1};
                for (int d = 0; d < 4; ++d) {
                    int nr = r + dr[d];
                    int nc = c + dc[d];
                    if (nr >= 0 && nr < rows_ && nc >= 0 && nc < cols_) {
                        VectorT nw = weights_.row(nr * cols_ + nc).transpose();
                        sum += (w - nw).norm();
                        ++count;
                    }
                }
                umat(r, c) = (count > 0) ? sum / count : 0;
            }
        }
        return umat;
    }

    /**
     * @brief Topographic error: fraction of samples where the 2nd-best
     *        BMU is not adjacent to the 1st BMU.
     */
    Scalar topographic_error(const MatrixT& data) const {
        int errors = 0;
        for (int i = 0; i < data.rows(); ++i) {
            VectorT sample = data.row(i).transpose();

            // Find top-2 BMUs
            int best = -1, second = -1;
            Scalar best_dist = std::numeric_limits<Scalar>::max();
            Scalar second_dist = std::numeric_limits<Scalar>::max();

            for (int j = 0; j < rows_ * cols_; ++j) {
                Scalar d = (weights_.row(j).transpose() - sample).squaredNorm();
                if (d < best_dist) {
                    second = best; second_dist = best_dist;
                    best = j;     best_dist = d;
                } else if (d < second_dist) {
                    second = j;   second_dist = d;
                }
            }

            // Check adjacency
            int r1 = best / cols_, c1 = best % cols_;
            int r2 = second / cols_, c2 = second % cols_;
            int manhattan = std::abs(r1 - r2) + std::abs(c1 - c2);
            if (manhattan > 1) ++errors;
        }
        return static_cast<Scalar>(errors) / static_cast<Scalar>(data.rows());
    }

    // ---------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int input_dim() const { return input_dim_; }
    int num_neurons() const { return rows_ * cols_; }

    /** @brief Get weight vector for grid cell (r, c). */
    VectorT weight(int r, int c) const {
        return weights_.row(r * cols_ + c).transpose();
    }

    /** @brief Get the full weight matrix (neurons × input_dim). */
    const MatrixT& weights() const { return weights_; }
    MatrixT& weights() { return weights_; }

    /** @brief Get grid position for flat index. */
    std::pair<int, int> grid_position(int flat_index) const {
        return {flat_index / cols_, flat_index % cols_};
    }

private:
    // ---------------------------------------------------------------
    // Decay functions
    // ---------------------------------------------------------------

    Scalar learning_rate(int t, int T) const {
        return initial_learning_rate_ *
               std::exp(-static_cast<Scalar>(t) / static_cast<Scalar>(T));
    }

    Scalar neighbourhood_radius(int t, int T) const {
        return initial_radius_ *
               std::exp(-static_cast<Scalar>(t) / time_constant_);
    }

    Scalar neighbourhood_influence(Scalar grid_dist, Scalar radius) const {
        if (radius <= 0) return 0;
        switch (neighbourhood_type_) {
        case NeighbourhoodType::GAUSSIAN:
            return std::exp(-(grid_dist * grid_dist) /
                            (2.0 * radius * radius));
        case NeighbourhoodType::BUBBLE:
            return (grid_dist <= radius) ? 1.0 : 0.0;
        default:
            return 0;
        }
    }

    // ---------------------------------------------------------------
    // Weight update
    // ---------------------------------------------------------------

    void update_weights(const VectorT& sample, const BMUResult& bmu,
                        Scalar lr, Scalar radius) {
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                Scalar dr = static_cast<Scalar>(r - bmu.row);
                Scalar dc = static_cast<Scalar>(c - bmu.col);
                Scalar grid_dist = std::sqrt(dr * dr + dc * dc);

                Scalar influence = neighbourhood_influence(grid_dist, radius);
                if (influence < 1e-10) continue;

                int idx = r * cols_ + c;
                weights_.row(idx) += lr * influence *
                    (sample.transpose() - weights_.row(idx));
            }
        }
    }

    // ---------------------------------------------------------------
    // Initialisation strategies
    // ---------------------------------------------------------------

    void init_random_uniform(const MatrixT& data) {
        VectorT mins = data.colwise().minCoeff().transpose();
        VectorT maxs = data.colwise().maxCoeff().transpose();

        std::uniform_real_distribution<Scalar> dist(0.0, 1.0);
        for (int i = 0; i < rows_ * cols_; ++i) {
            for (int d = 0; d < input_dim_; ++d) {
                weights_(i, d) = mins(d) + dist(rng_) * (maxs(d) - mins(d));
            }
        }
    }

    void init_random_sample(const MatrixT& data) {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(data.rows()) - 1);
        for (int i = 0; i < rows_ * cols_; ++i) {
            weights_.row(i) = data.row(dist(rng_));
        }
    }

    void init_pca(const MatrixT& data) {
        // Centre data
        VectorT mean = data.colwise().mean().transpose();
        MatrixT centred = data.rowwise() - mean.transpose();

        // Compute covariance and top-2 eigenvectors
        MatrixT cov = (centred.transpose() * centred) /
                      static_cast<Scalar>(data.rows() - 1);
        Eigen::SelfAdjointEigenSolver<MatrixT> solver(cov);
        MatrixT eigvecs = solver.eigenvectors();

        // Take the two largest eigenvectors
        VectorT pc1 = eigvecs.col(input_dim_ - 1);
        VectorT pc2 = (input_dim_ >= 2) ? VectorT(eigvecs.col(input_dim_ - 2))
                                         : VectorT::Zero(input_dim_);

        // Spread weights along PC1 and PC2
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                Scalar t1 = (rows_ > 1)
                    ? (static_cast<Scalar>(r) / (rows_ - 1) - 0.5) * 2
                    : 0;
                Scalar t2 = (cols_ > 1)
                    ? (static_cast<Scalar>(c) / (cols_ - 1) - 0.5) * 2
                    : 0;
                VectorT w = mean + t1 * pc1 + t2 * pc2;
                weights_.row(r * cols_ + c) = w.transpose();
            }
        }
    }
};

} // namespace ml
