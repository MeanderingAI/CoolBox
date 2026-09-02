#ifndef COOLBOX__LIBRARIES_PACKAGES_DM_MATRIX_PROFILE_HEADERS_MATRIX_PROFILE_HPP
#define COOLBOX__LIBRARIES_PACKAGES_DM_MATRIX_PROFILE_HEADERS_MATRIX_PROFILE_HPP

#include "matrix_dense.h"

#include <cstddef>
#include <limits>
#include <vector>

namespace dm {
namespace matrix_profile {

// ─── Core result types ────────────────────────────────────────────────────────

// The matrix profile of a time series:
//   profile[i] = z-normalised Euclidean distance from subsequence i to its nearest
//                non-trivial neighbour.
//   index[i]   = starting index of that nearest neighbour.
struct MatrixProfile {
    std::vector<double>      profile;
    std::vector<std::size_t> index;
    std::size_t              subsequence_length = 0U;
};

// A motif is a pair of similar (low-distance) subsequences.
struct Motif {
    std::size_t index_a;
    std::size_t index_b;
    double      distance;
};

// A discord is an anomalous (high-distance) subsequence.
struct Discord {
    std::size_t index;
    double      distance;
};

// ─── Preprocessing ────────────────────────────────────────────────────────────

// Z-normalise a vector: subtract mean, divide by standard deviation.
// Returns a zero vector if the input has near-zero variance.
std::vector<double> z_normalize(const std::vector<double>& subsequence);

// Compute the running means and standard deviations for all length-m subsequences
// of time_series in a single O(n) pass.
void sliding_statistics(const std::vector<double>& time_series,
                        std::size_t                subsequence_length,
                        std::vector<double>&       out_means,
                        std::vector<double>&       out_stds);

// ─── Distance profile ────────────────────────────────────────────────────────

// Compute the z-normalised Euclidean distance profile between a single query
// subsequence (already extracted from position query_index in time_series) and
// every subsequence in time_series.
// Uses the MASS (Mueen's Algorithm for Similarity Search) inner-product form:
//   d(i,j) = sqrt( 2m (1 − pearson(i,j)) )
std::vector<double> distance_profile(const std::vector<double>& time_series,
                                     std::size_t                query_index,
                                     std::size_t                subsequence_length,
                                     const std::vector<double>& means,
                                     const std::vector<double>& stds);

// ─── Matrix profile algorithms ────────────────────────────────────────────────

// Self-join (STOMP): compute the matrix profile of time_series against itself.
// exclusion_zone == 0  ⟹  auto-set to  m / 4  (standard Keogh recommendation).
MatrixProfile self_join(const std::vector<double>& time_series,
                        std::size_t                subsequence_length,
                        std::size_t                exclusion_zone = 0U);

// AB-join: compute the matrix profile of ts_a against ts_b.
// For each subsequence in ts_a, finds its nearest neighbour in ts_b.
// No exclusion zone is applied (the two series are assumed independent).
MatrixProfile ab_join(const std::vector<double>& ts_a,
                      const std::vector<double>& ts_b,
                      std::size_t                subsequence_length);

// ─── Post-processing ──────────────────────────────────────────────────────────

// Return the top-k motif pairs sorted by ascending distance.
// Pairs where either index is within exclusion_zone of an already-returned pair
// are suppressed.
std::vector<Motif> top_motifs(const MatrixProfile& mp,
                               std::size_t          k,
                               std::size_t          exclusion_zone = 0U);

// Return the top-k discords sorted by descending distance.
// Indices within exclusion_zone of an already-returned discord are suppressed.
std::vector<Discord> top_discords(const MatrixProfile& mp,
                                   std::size_t          k,
                                   std::size_t          exclusion_zone = 0U);

// ─── Segmentation (FLUSS / FLOSS arc curve) ───────────────────────────────────

// Compute the (corrected) arc curve CAC from the matrix profile index.
// The arc curve counts, for each position t, how many arcs cross over t.
// It is normalised so values are in [0, 1], with low values indicating regime changes.
std::vector<double> arc_curve(const MatrixProfile& mp);

// Return up to num_segments − 1 regime-change boundary positions as the
// local minima of the arc curve with the lowest CAC values.
std::vector<std::size_t> segmentation_points(const std::vector<double>& arc,
                                              std::size_t                num_segments);

// ─── Utility ─────────────────────────────────────────────────────────────────

// Extract the subsequence starting at position index of length length from ts.
std::vector<double> extract_subsequence(const std::vector<double>& time_series,
                                        std::size_t                index,
                                        std::size_t                length);

// Compute the matrix profile for every row of a DenseMatrix.
// Each row is treated as a separate time series.
std::vector<MatrixProfile> batch_self_join(const mytrix::DenseMatrix& matrix,
                                           std::size_t                subsequence_length);

}  // namespace matrix_profile
}  // namespace dm

#endif  // COOLBOX__LIBRARIES_PACKAGES_DM_MATRIX_PROFILE_HEADERS_MATRIX_PROFILE_HPP
