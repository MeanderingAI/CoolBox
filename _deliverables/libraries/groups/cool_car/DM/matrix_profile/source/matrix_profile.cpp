#include "matrix_profile.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace dm {
namespace matrix_profile {

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kEps = 1e-10;

// Compute the initial dot product QT[j] = sum_{k=0}^{m-1} ts[k] * ts[j+k]
// for row i=0 and all valid j in [0, n_subseq).
std::vector<double> initial_dot_products(const std::vector<double>& ts,
                                         std::size_t                m,
                                         std::size_t                n_subseq) {
    std::vector<double> qt(n_subseq, 0.0);
    for (std::size_t j = 0; j < n_subseq; ++j) {
        for (std::size_t k = 0; k < m; ++k) {
            qt[j] += ts[k] * ts[j + k];
        }
    }
    return qt;
}

// Compute pearson correlation from a pre-computed dot product QT_{i,j}.
// Returns the z-normalised distance d(i,j).
double qt_to_distance(double qt_ij, double mean_i, double std_i,
                      double mean_j, double std_j, std::size_t m) {
    if (std_i < kEps || std_j < kEps) {
        // Both constant or one of them is — define as distance 0 if both constant
        return (std_i < kEps && std_j < kEps) ? 0.0 : std::sqrt(2.0 * static_cast<double>(m));
    }
    double pearson = (qt_ij / static_cast<double>(m) - mean_i * mean_j) / (std_i * std_j);
    pearson = std::max(-1.0, std::min(1.0, pearson));
    return std::sqrt(std::max(0.0, 2.0 * static_cast<double>(m) * (1.0 - pearson)));
}

}  // namespace

// ─── Preprocessing ────────────────────────────────────────────────────────────

std::vector<double> z_normalize(const std::vector<double>& s) {
    const std::size_t n = s.size();
    if (n == 0U) {
        return {};
    }
    double sum = 0.0;
    double sum_sq = 0.0;
    for (double v : s) {
        sum    += v;
        sum_sq += v * v;
    }
    const double mean  = sum / static_cast<double>(n);
    const double var   = sum_sq / static_cast<double>(n) - mean * mean;
    const double sigma = std::sqrt(std::max(0.0, var));

    std::vector<double> out(n);
    if (sigma < kEps) {
        return out;  // zero vector
    }
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = (s[i] - mean) / sigma;
    }
    return out;
}

void sliding_statistics(const std::vector<double>& ts,
                        std::size_t                m,
                        std::vector<double>&       out_means,
                        std::vector<double>&       out_stds) {
    const std::size_t n = ts.size();
    if (n < m) {
        out_means.clear();
        out_stds.clear();
        return;
    }
    const std::size_t n_subseq = n - m + 1U;
    out_means.resize(n_subseq);
    out_stds.resize(n_subseq);

    double sum    = 0.0;
    double sum_sq = 0.0;
    for (std::size_t k = 0; k < m; ++k) {
        sum    += ts[k];
        sum_sq += ts[k] * ts[k];
    }
    out_means[0] = sum / static_cast<double>(m);
    out_stds[0]  = std::sqrt(std::max(0.0, sum_sq / static_cast<double>(m) -
                                           out_means[0] * out_means[0]));

    for (std::size_t i = 1; i < n_subseq; ++i) {
        sum    += ts[i + m - 1U] - ts[i - 1U];
        sum_sq += ts[i + m - 1U] * ts[i + m - 1U] - ts[i - 1U] * ts[i - 1U];
        out_means[i] = sum / static_cast<double>(m);
        out_stds[i]  = std::sqrt(std::max(0.0, sum_sq / static_cast<double>(m) -
                                               out_means[i] * out_means[i]));
    }
}

// ─── Distance profile ────────────────────────────────────────────────────────

std::vector<double> distance_profile(const std::vector<double>& ts,
                                     std::size_t                qi,
                                     std::size_t                m,
                                     const std::vector<double>& means,
                                     const std::vector<double>& stds) {
    const std::size_t n_subseq = ts.size() - m + 1U;
    if (qi >= n_subseq) {
        throw std::out_of_range("distance_profile: query_index out of range");
    }
    std::vector<double> dp(n_subseq);
    for (std::size_t j = 0; j < n_subseq; ++j) {
        double qt = 0.0;
        for (std::size_t k = 0; k < m; ++k) {
            qt += ts[qi + k] * ts[j + k];
        }
        dp[j] = qt_to_distance(qt, means[qi], stds[qi], means[j], stds[j], m);
    }
    return dp;
}

// ─── Self-join (STOMP) ────────────────────────────────────────────────────────

MatrixProfile self_join(const std::vector<double>& ts,
                        std::size_t                m,
                        std::size_t                exclusion_zone) {
    const std::size_t n = ts.size();
    if (n < m) {
        throw std::invalid_argument("self_join: time series shorter than subsequence_length");
    }
    if (m == 0U) {
        throw std::invalid_argument("self_join: subsequence_length must be > 0");
    }

    const std::size_t n_subseq = n - m + 1U;
    const std::size_t excl = (exclusion_zone == 0U) ? (m / 4U + 1U) : exclusion_zone;

    // Precompute sliding statistics
    std::vector<double> means, stds;
    sliding_statistics(ts, m, means, stds);

    // Initialise matrix profile and index
    std::vector<double>      mp(n_subseq, kInf);
    std::vector<std::size_t> mpi(n_subseq, 0U);

    // Initial dot products: QT[j] = dot(T[0:m], T[j:j+m])
    std::vector<double> qt = initial_dot_products(ts, m, n_subseq);

    // Save first row to restart the sliding QT at each diagonal
    const std::vector<double> qt_first = qt;

    // ── STOMP: iterate over each query position i ──────────────────────────
    for (std::size_t i = 0; i < n_subseq; ++i) {
        if (i > 0U) {
            // Update QT in-place (right to left to avoid using updated values)
            // QT_{i,j} = QT_{i-1, j-1} + ts[i+m-1]*ts[j+m-1] - ts[i-1]*ts[j-1]
            for (std::size_t j = n_subseq - 1U; j >= 1U; --j) {
                qt[j] = qt[j - 1U]
                      + ts[i + m - 1U] * ts[j + m - 1U]
                      - ts[i - 1U]     * ts[j - 1U];
            }
            // QT_{i,0} = dot(T[i:i+m], T[0:m]) — use first-row of the original
            // dot products shifted: it equals qt_first[i]
            qt[0] = qt_first[i];
        }

        // Compute distance for each candidate j and update both sides of MP
        for (std::size_t j = 0; j < n_subseq; ++j) {
            // Skip trivial matches
            const std::size_t diff = (i >= j) ? (i - j) : (j - i);
            if (diff <= excl) {
                continue;
            }
            const double dist = qt_to_distance(qt[j], means[i], stds[i],
                                               means[j], stds[j], m);
            if (dist < mp[i]) {
                mp[i]  = dist;
                mpi[i] = j;
            }
            if (dist < mp[j]) {
                mp[j]  = dist;
                mpi[j] = i;
            }
        }
    }

    return MatrixProfile{mp, mpi, m};
}

// ─── AB-join ──────────────────────────────────────────────────────────────────

MatrixProfile ab_join(const std::vector<double>& ts_a,
                      const std::vector<double>& ts_b,
                      std::size_t                m) {
    if (ts_a.size() < m || ts_b.size() < m) {
        throw std::invalid_argument("ab_join: time series shorter than subsequence_length");
    }
    if (m == 0U) {
        throw std::invalid_argument("ab_join: subsequence_length must be > 0");
    }

    const std::size_t na = ts_a.size() - m + 1U;
    const std::size_t nb = ts_b.size() - m + 1U;

    std::vector<double> means_a, stds_a, means_b, stds_b;
    sliding_statistics(ts_a, m, means_a, stds_a);
    sliding_statistics(ts_b, m, means_b, stds_b);

    std::vector<double>      mp(na, kInf);
    std::vector<std::size_t> mpi(na, 0U);

    // Initial dot products: QT[j] = dot(A[0:m], B[j:j+m])
    std::vector<double> qt(nb, 0.0);
    for (std::size_t j = 0; j < nb; ++j) {
        for (std::size_t k = 0; k < m; ++k) {
            qt[j] += ts_a[k] * ts_b[j + k];
        }
    }
    const std::vector<double> qt_first = qt;  // QT_{0, *}

    for (std::size_t i = 0; i < na; ++i) {
        if (i > 0U) {
            for (std::size_t j = nb - 1U; j >= 1U; --j) {
                qt[j] = qt[j - 1U]
                      + ts_a[i + m - 1U] * ts_b[j + m - 1U]
                      - ts_a[i - 1U]     * ts_b[j - 1U];
            }
            qt[0] = qt_first[i < nb ? i : nb - 1U];
            // Recompute qt[0] properly: dot(A[i:i+m], B[0:m])
            qt[0] = 0.0;
            for (std::size_t k = 0; k < m; ++k) {
                qt[0] += ts_a[i + k] * ts_b[k];
            }
        }

        for (std::size_t j = 0; j < nb; ++j) {
            const double dist = qt_to_distance(qt[j], means_a[i], stds_a[i],
                                               means_b[j], stds_b[j], m);
            if (dist < mp[i]) {
                mp[i]  = dist;
                mpi[i] = j;
            }
        }
    }

    return MatrixProfile{mp, mpi, m};
}

// ─── Post-processing ──────────────────────────────────────────────────────────

std::vector<Motif> top_motifs(const MatrixProfile& mp,
                               std::size_t          k,
                               std::size_t          exclusion_zone) {
    const std::size_t n = mp.profile.size();
    const std::size_t excl = (exclusion_zone == 0U)
                             ? (mp.subsequence_length / 4U + 1U)
                             : exclusion_zone;

    // Build a sorted index by profile value (ascending)
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), 0U);
    std::sort(order.begin(), order.end(),
              [&](std::size_t a, std::size_t b) {
                  return mp.profile[a] < mp.profile[b];
              });

    std::vector<Motif> motifs;
    motifs.reserve(k);
    std::vector<bool> used(n, false);

    for (std::size_t rank = 0; rank < n && motifs.size() < k; ++rank) {
        const std::size_t i = order[rank];
        if (used[i]) {
            continue;
        }
        const std::size_t j = mp.index[i];
        if (used[j]) {
            continue;
        }
        if (mp.profile[i] >= kInf) {
            break;
        }
        motifs.push_back({i, j, mp.profile[i]});

        // Mark neighbourhood of both indices as used
        const std::size_t lo_i = (i >= excl) ? (i - excl) : 0U;
        const std::size_t hi_i = std::min(n, i + excl + 1U);
        for (std::size_t t = lo_i; t < hi_i; ++t) { used[t] = true; }
        const std::size_t lo_j = (j >= excl) ? (j - excl) : 0U;
        const std::size_t hi_j = std::min(n, j + excl + 1U);
        for (std::size_t t = lo_j; t < hi_j; ++t) { used[t] = true; }
    }

    return motifs;
}

std::vector<Discord> top_discords(const MatrixProfile& mp,
                                   std::size_t          k,
                                   std::size_t          exclusion_zone) {
    const std::size_t n = mp.profile.size();
    const std::size_t excl = (exclusion_zone == 0U)
                             ? (mp.subsequence_length / 4U + 1U)
                             : exclusion_zone;

    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), 0U);
    std::sort(order.begin(), order.end(),
              [&](std::size_t a, std::size_t b) {
                  return mp.profile[a] > mp.profile[b];
              });

    std::vector<Discord> discords;
    discords.reserve(k);
    std::vector<bool> used(n, false);

    for (std::size_t rank = 0; rank < n && discords.size() < k; ++rank) {
        const std::size_t i = order[rank];
        if (used[i]) {
            continue;
        }
        if (mp.profile[i] <= 0.0) {
            break;
        }
        discords.push_back({i, mp.profile[i]});

        const std::size_t lo = (i >= excl) ? (i - excl) : 0U;
        const std::size_t hi = std::min(n, i + excl + 1U);
        for (std::size_t t = lo; t < hi; ++t) { used[t] = true; }
    }

    return discords;
}

// ─── Segmentation ─────────────────────────────────────────────────────────────

std::vector<double> arc_curve(const MatrixProfile& mp) {
    const std::size_t n = mp.profile.size();
    if (n == 0U) {
        return {};
    }

    // Count arcs: an arc crosses position t if min(i, index[i]) < t <= max(i, index[i]).
    std::vector<double> arc(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        if (mp.profile[i] >= kInf) {
            continue;
        }
        const std::size_t j   = mp.index[i];
        const std::size_t lo  = std::min(i, j) + 1U;
        const std::size_t hi  = std::max(i, j);
        for (std::size_t t = lo; t < hi && t < n; ++t) {
            arc[t] += 1.0;
        }
    }

    // Normalise by the ideal arc count at each position (triangular expected value)
    const double dn = static_cast<double>(n);
    for (std::size_t t = 0; t < n; ++t) {
        const double ideal = 2.0 * static_cast<double>(t + 1U) *
                             static_cast<double>(n - t) / (dn * dn);
        if (ideal > kEps) {
            arc[t] /= (ideal * dn);
        }
        arc[t] = std::min(1.0, arc[t]);
    }
    return arc;
}

std::vector<std::size_t> segmentation_points(const std::vector<double>& arc,
                                              std::size_t                num_segments) {
    if (num_segments <= 1U || arc.empty()) {
        return {};
    }
    const std::size_t want     = num_segments - 1U;
    const std::size_t n        = arc.size();
    const std::size_t min_dist = n / (num_segments * 2U + 1U);

    // Collect local minima
    std::vector<std::size_t> candidates;
    for (std::size_t t = 1; t + 1U < n; ++t) {
        if (arc[t] <= arc[t - 1U] && arc[t] <= arc[t + 1U]) {
            candidates.push_back(t);
        }
    }

    // Sort candidates by arc value (ascending)
    std::sort(candidates.begin(), candidates.end(),
              [&](std::size_t a, std::size_t b) {
                  return arc[a] < arc[b];
              });

    std::vector<std::size_t> result;
    result.reserve(want);
    std::vector<bool> used(n, false);

    for (std::size_t ci = 0; ci < candidates.size() && result.size() < want; ++ci) {
        const std::size_t t = candidates[ci];
        if (used[t]) {
            continue;
        }
        result.push_back(t);
        const std::size_t lo = (t >= min_dist) ? (t - min_dist) : 0U;
        const std::size_t hi = std::min(n, t + min_dist + 1U);
        for (std::size_t k = lo; k < hi; ++k) { used[k] = true; }
    }

    std::sort(result.begin(), result.end());
    return result;
}

// ─── Utility ─────────────────────────────────────────────────────────────────

std::vector<double> extract_subsequence(const std::vector<double>& ts,
                                        std::size_t                index,
                                        std::size_t                length) {
    if (index + length > ts.size()) {
        throw std::out_of_range("extract_subsequence: index + length exceeds time series length");
    }
    return std::vector<double>(ts.begin() + static_cast<std::ptrdiff_t>(index),
                               ts.begin() + static_cast<std::ptrdiff_t>(index + length));
}

std::vector<MatrixProfile> batch_self_join(const mytrix::DenseMatrix& mat,
                                           std::size_t                m) {
    const std::size_t n_rows = static_cast<std::size_t>(mat.rows());
    const std::size_t n_cols = static_cast<std::size_t>(mat.cols());
    std::vector<MatrixProfile> results;
    results.reserve(n_rows);
    for (std::size_t r = 0; r < n_rows; ++r) {
        std::vector<double> row(n_cols);
        for (std::size_t c = 0; c < n_cols; ++c) {
            row[c] = mat.at(r, c);
        }
        results.push_back(self_join(row, m));
    }
    return results;
}

}  // namespace matrix_profile
}  // namespace dm
