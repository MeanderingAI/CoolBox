#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

namespace trekker {
namespace algorithm {
namespace dynamic_programming {

template <typename T>
class RankConvergenceLTDP {
public:
    using value_type = T;
    using Vector = std::vector<T>;
    using Matrix = std::vector<Vector>;   // row-major
    using Stages = std::vector<Matrix>;   // A_1 .. A_n
    using PredVector = std::vector<std::size_t>;
    using PredStages = std::vector<PredVector>; // pred[0] unused for 1-based indexing

    struct ForwardResult {
        std::vector<Vector> stage_vectors; // stage_vectors[0] = s0, size n + 1
        PredStages predecessors;           // predecessors[0] unused, size n + 1
        std::size_t fixup_iterations = 0;
    };

    static constexpr T neg_inf() {
        return -std::numeric_limits<T>::infinity();
    }

    static bool is_all_non_zero(const Vector& vector) {
        for (const T value : vector) {
            if (value == neg_inf()) {
                return false;
            }
        }
        return true;
    }

    static bool are_parallel(const Vector& a, const Vector& b, T epsilon = static_cast<T>(1e-8)) {
        if (a.size() != b.size()) {
            return false;
        }

        bool found_anchor = false;
        T offset = static_cast<T>(0);

        for (std::size_t i = 0; i < a.size(); ++i) {
            const bool a_inf = (a[i] == neg_inf());
            const bool b_inf = (b[i] == neg_inf());
            if (a_inf || b_inf) {
                if (a_inf != b_inf) {
                    return false;
                }
                continue;
            }

            const T current_offset = b[i] - a[i];
            if (!found_anchor) {
                offset = current_offset;
                found_anchor = true;
            } else if (std::fabs(current_offset - offset) > epsilon) {
                return false;
            }
        }

        // Both vectors can be all -inf and still be considered parallel.
        return true;
    }

    static ForwardResult forward_sequential(const Stages& stages, const Vector& s0) {
        validate_inputs(stages, s0);

        const std::size_t n = stages.size();
        ForwardResult result;
        result.stage_vectors.assign(n + 1, Vector{});
        result.predecessors.assign(n + 1, PredVector{});

        result.stage_vectors[0] = s0;
        Vector current = s0;

        for (std::size_t i = 1; i <= n; ++i) {
            result.predecessors[i] = predecessor_product(stages[i - 1], current);
            current = multiply(stages[i - 1], current);
            result.stage_vectors[i] = current;
        }

        return result;
    }

    // Implementation of Figure 4 from the paper; parallel regions are simulated
    // with processor-partitioned loops to preserve algorithmic behavior.
    static ForwardResult forward_rank_convergence(
        const Stages& stages,
        const Vector& s0,
        std::size_t processors,
        const Vector& nz,
        std::size_t max_fixup_iterations = 0) {
        validate_inputs(stages, s0);
        if (processors == 0) {
            throw std::invalid_argument("processors must be > 0");
        }
        if (nz.size() != s0.size()) {
            throw std::invalid_argument("nz size must match s0 size");
        }
        if (!is_all_non_zero(nz)) {
            throw std::invalid_argument("nz must be all-non-zero (-inf is not allowed)");
        }

        const std::size_t n = stages.size();
        processors = std::min(processors, n == 0 ? std::size_t(1) : n);
        const std::vector<std::pair<std::size_t, std::size_t>> ranges = build_ranges(n, processors);

        ForwardResult result;
        result.stage_vectors.assign(n + 1, Vector{});
        result.predecessors.assign(n + 1, PredVector{});
        result.stage_vectors[0] = s0;

        // Initial forward pass per partition.
        for (std::size_t p = 0; p < processors; ++p) {
            const std::size_t l = ranges[p].first;
            const std::size_t r = ranges[p].second;
            Vector local = (p == 0) ? s0 : nz;
            for (std::size_t i = l + 1; i <= r; ++i) {
                result.predecessors[i] = predecessor_product(stages[i - 1], local);
                local = multiply(stages[i - 1], local);
                result.stage_vectors[i] = local;
            }
        }

        if (processors <= 1) {
            return result;
        }

        std::vector<bool> converged(processors, false);
        converged[0] = true;

        const std::size_t iteration_cap =
            (max_fixup_iterations == 0) ? std::max<std::size_t>(processors - 1, 1) * std::max<std::size_t>(n, 1)
                                        : max_fixup_iterations;

        for (std::size_t iteration = 1; iteration <= iteration_cap; ++iteration) {
            for (std::size_t p = 1; p < processors; ++p) {
                converged[p] = false;
                const std::size_t l = ranges[p].first;
                const std::size_t r = ranges[p].second;
                Vector local = result.stage_vectors[l];

                for (std::size_t i = l + 1; i <= r; ++i) {
                    result.predecessors[i] = predecessor_product(stages[i - 1], local);
                    const Vector next = multiply(stages[i - 1], local);

                    if (are_parallel(next, result.stage_vectors[i])) {
                        converged[p] = true;
                        break;
                    }

                    result.stage_vectors[i] = next;
                    local = next;
                }
            }

            result.fixup_iterations = iteration;
            if (std::all_of(converged.begin(), converged.end(), [](bool value) { return value; })) {
                return result;
            }
        }

        throw std::runtime_error("rank-convergence fix-up did not converge within iteration cap");
    }

    // Parallel vectors produce identical predecessor products (paper lemma).
    static PredVector predecessor_product(const Matrix& matrix, const Vector& vector) {
        validate_matrix_vector(matrix, vector);

        const std::size_t rows = matrix.size();
        const std::size_t cols = vector.size();
        PredVector pred(rows, 0);

        for (std::size_t i = 0; i < rows; ++i) {
            T best = neg_inf();
            std::size_t arg = 0;
            for (std::size_t k = 0; k < cols; ++k) {
                const T score = plus_tropical(matrix[i][k], vector[k]);
                if (score > best) {
                    best = score;
                    arg = k;
                }
            }
            pred[i] = arg;
        }

        return pred;
    }

    static Vector multiply(const Matrix& matrix, const Vector& vector) {
        validate_matrix_vector(matrix, vector);

        const std::size_t rows = matrix.size();
        const std::size_t cols = vector.size();
        Vector out(rows, neg_inf());

        for (std::size_t i = 0; i < rows; ++i) {
            T best = neg_inf();
            for (std::size_t k = 0; k < cols; ++k) {
                best = std::max(best, plus_tropical(matrix[i][k], vector[k]));
            }
            out[i] = best;
        }

        return out;
    }

private:
    static T plus_tropical(T a, T b) {
        if (a == neg_inf() || b == neg_inf()) {
            return neg_inf();
        }
        return a + b;
    }

    static void validate_matrix_vector(const Matrix& matrix, const Vector& vector) {
        if (matrix.empty() || vector.empty()) {
            throw std::invalid_argument("matrix and vector must be non-empty");
        }
        const std::size_t cols = vector.size();
        for (const auto& row : matrix) {
            if (row.size() != cols) {
                throw std::invalid_argument("matrix column count must match vector size");
            }
            bool non_trivial = false;
            for (const T value : row) {
                if (value != neg_inf()) {
                    non_trivial = true;
                    break;
                }
            }
            if (!non_trivial) {
                throw std::invalid_argument("all rows must be non-trivial (at least one finite entry)");
            }
        }
    }

    static void validate_inputs(const Stages& stages, const Vector& s0) {
        if (stages.empty() || s0.empty()) {
            throw std::invalid_argument("stages and s0 must be non-empty");
        }
        for (const auto& matrix : stages) {
            validate_matrix_vector(matrix, s0);
        }
    }

    static std::vector<std::pair<std::size_t, std::size_t>> build_ranges(
        std::size_t n,
        std::size_t processors) {
        std::vector<std::pair<std::size_t, std::size_t>> ranges;
        ranges.reserve(processors);

        std::size_t cursor = 0;
        for (std::size_t p = 0; p < processors; ++p) {
            const std::size_t remain_stages = n - cursor;
            const std::size_t remain_procs = processors - p;
            const std::size_t chunk = remain_stages / remain_procs;
            const std::size_t next = cursor + chunk;
            ranges.push_back({cursor, next});
            cursor = next;
        }

        return ranges;
    }
};

} // namespace dynamic_programming
} // namespace algorithm
} // namespace trekker
