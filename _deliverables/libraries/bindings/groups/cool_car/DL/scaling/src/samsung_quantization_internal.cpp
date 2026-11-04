#include "samsung_quantization_internal.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace samsung_internal {
namespace {

double& at(Tensor& x, size_t row, size_t col) {
    return x.data()[row * x.shape()[1] + col];
}
double at(const Tensor& x, size_t row, size_t col) {
    return x.data()[row * x.shape()[1] + col];
}

void normalize_column(Tensor& x, size_t column) {
    double norm = 0.0;
    for (size_t i = 0; i < x.shape()[0]; ++i) norm += at(x, i, column) * at(x, i, column);
    norm = std::sqrt(norm);
    if (norm == 0.0) return;
    for (size_t i = 0; i < x.shape()[0]; ++i) at(x, i, column) /= norm;
}

} // namespace

void validate_matrix(const Tensor& matrix, const char* method) {
    if (matrix.ndim() != 2 || matrix.shape()[0] == 0 || matrix.shape()[1] == 0) {
        throw std::invalid_argument(std::string(method) + ": expected a non-empty 2-D matrix");
    }
    for (double value : matrix.data()) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(std::string(method) + ": matrix must contain finite values");
        }
    }
}

Tensor transpose(const Tensor& matrix) {
    return matrix.transpose();
}

Tensor matmul(const Tensor& left, const Tensor& right) {
    return left.matmul(right);
}

Tensor identity(size_t size) {
    Tensor result({size, size}, 0.0);
    for (size_t i = 0; i < size; ++i) at(result, i, i) = 1.0;
    return result;
}

Tensor sign(const Tensor& matrix) {
    Tensor result(matrix.shape(), 1.0);
    for (size_t i = 0; i < matrix.size(); ++i) {
        result.data()[i] = matrix.data()[i] < 0.0 ? -1.0 : 1.0;
    }
    return result;
}

Svd truncated_svd(const Tensor& matrix, size_t rank) {
    validate_matrix(matrix, "truncated_svd");
    const size_t rows = matrix.shape()[0];
    const size_t cols = matrix.shape()[1];
    const size_t limit = std::min(rows, cols);
    if (rank == 0 || rank > limit) {
        throw std::invalid_argument("truncated_svd: rank must be in [1, min(rows, cols)]");
    }

    // Jacobi diagonalization of the smaller Gram matrix avoids a mandatory
    // BLAS/LAPACK dependency in the standalone package.
    const bool right_gram = cols <= rows;
    const size_t n = right_gram ? cols : rows;
    Tensor gram({n, n}, 0.0);
    if (right_gram) {
        gram = matmul(transpose(matrix), matrix);
    } else {
        gram = matmul(matrix, transpose(matrix));
    }
    Tensor vectors = identity(n);

    const size_t sweeps = std::max<size_t>(32, n * n * 8);
    for (size_t iteration = 0; iteration < sweeps; ++iteration) {
        size_t p = 0, q = 0;
        double largest = 0.0;
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                if (std::abs(at(gram, i, j)) > largest) {
                    largest = std::abs(at(gram, i, j));
                    p = i;
                    q = j;
                }
            }
        }
        double diagonal_scale = 1.0;
        for (size_t i = 0; i < n; ++i) diagonal_scale += std::abs(at(gram, i, i));
        if (largest <= 1e-14 * diagonal_scale) break;

        const double app = at(gram, p, p);
        const double aqq = at(gram, q, q);
        const double apq = at(gram, p, q);
        const double angle = 0.5 * std::atan2(2.0 * apq, aqq - app);
        const double c = std::cos(angle);
        const double s = std::sin(angle);

        for (size_t k = 0; k < n; ++k) {
            const double gkp = at(gram, k, p);
            const double gkq = at(gram, k, q);
            at(gram, k, p) = c * gkp - s * gkq;
            at(gram, k, q) = s * gkp + c * gkq;
        }
        for (size_t k = 0; k < n; ++k) {
            const double gpk = at(gram, p, k);
            const double gqk = at(gram, q, k);
            at(gram, p, k) = c * gpk - s * gqk;
            at(gram, q, k) = s * gpk + c * gqk;
        }
        at(gram, p, q) = at(gram, q, p) = 0.0;
        for (size_t k = 0; k < n; ++k) {
            const double vkp = at(vectors, k, p);
            const double vkq = at(vectors, k, q);
            at(vectors, k, p) = c * vkp - s * vkq;
            at(vectors, k, q) = s * vkp + c * vkq;
        }
    }

    std::vector<size_t> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return at(gram, a, a) > at(gram, b, b);
    });

    Tensor u({rows, rank}, 0.0);
    Tensor vt({rank, cols}, 0.0);
    std::vector<double> singular(rank, 0.0);
    for (size_t component = 0; component < rank; ++component) {
        const size_t source = order[component];
        const double sigma = std::sqrt(std::max(0.0, at(gram, source, source)));
        singular[component] = sigma;
        if (right_gram) {
            for (size_t j = 0; j < cols; ++j) at(vt, component, j) = at(vectors, j, source);
            if (sigma > 1e-15) {
                for (size_t i = 0; i < rows; ++i)
                    for (size_t j = 0; j < cols; ++j)
                        at(u, i, component) += at(matrix, i, j) * at(vt, component, j) / sigma;
            } else {
                double best_norm = -1.0;
                std::vector<double> best(rows, 0.0);
                for (size_t candidate = 0; candidate < rows; ++candidate) {
                    std::vector<double> trial(rows, 0.0);
                    trial[candidate] = 1.0;
                    for (size_t previous = 0; previous < component; ++previous) {
                        double dot = 0.0;
                        for (size_t i = 0; i < rows; ++i) dot += trial[i] * at(u, i, previous);
                        for (size_t i = 0; i < rows; ++i) trial[i] -= dot * at(u, i, previous);
                    }
                    const double norm = std::inner_product(
                        trial.begin(), trial.end(), trial.begin(), 0.0);
                    if (norm > best_norm) {
                        best_norm = norm;
                        best = std::move(trial);
                    }
                }
                for (size_t i = 0; i < rows; ++i) at(u, i, component) = best[i];
            }
            normalize_column(u, component);
        } else {
            for (size_t i = 0; i < rows; ++i) at(u, i, component) = at(vectors, i, source);
            if (sigma > 1e-15) {
                for (size_t j = 0; j < cols; ++j)
                    for (size_t i = 0; i < rows; ++i)
                        at(vt, component, j) += at(u, i, component) * at(matrix, i, j) / sigma;
            } else {
                double best_norm = -1.0;
                std::vector<double> best(cols, 0.0);
                for (size_t candidate = 0; candidate < cols; ++candidate) {
                    std::vector<double> trial(cols, 0.0);
                    trial[candidate] = 1.0;
                    for (size_t previous = 0; previous < component; ++previous) {
                        double dot = 0.0;
                        for (size_t j = 0; j < cols; ++j) dot += trial[j] * at(vt, previous, j);
                        for (size_t j = 0; j < cols; ++j) trial[j] -= dot * at(vt, previous, j);
                    }
                    const double norm = std::inner_product(
                        trial.begin(), trial.end(), trial.begin(), 0.0);
                    if (norm > best_norm) {
                        best_norm = norm;
                        best = std::move(trial);
                    }
                }
                for (size_t j = 0; j < cols; ++j) at(vt, component, j) = best[j];
            }
            double norm = 0.0;
            for (size_t j = 0; j < cols; ++j) norm += at(vt, component, j) * at(vt, component, j);
            norm = std::sqrt(norm);
            if (norm > 0.0) for (size_t j = 0; j < cols; ++j) at(vt, component, j) /= norm;
        }

        // Resolve the SVD sign ambiguity reproducibly.
        size_t pivot = 0;
        for (size_t i = 1; i < rows; ++i)
            if (std::abs(at(u, i, component)) > std::abs(at(u, pivot, component))) pivot = i;
        if (at(u, pivot, component) < 0.0) {
            for (size_t i = 0; i < rows; ++i) at(u, i, component) *= -1.0;
            for (size_t j = 0; j < cols; ++j) at(vt, component, j) *= -1.0;
        }
    }
    return {u, singular, vt};
}

Tensor rank_one_magnitude(const Tensor& matrix, size_t iterations) {
    validate_matrix(matrix, "rank_one_magnitude");
    if (iterations == 0) throw std::invalid_argument("rank_one_magnitude: iterations must be > 0");
    const size_t rows = matrix.shape()[0], cols = matrix.shape()[1];
    std::vector<double> v(cols, 1.0 / std::sqrt(static_cast<double>(cols)));
    std::vector<double> u(rows, 0.0);
    for (size_t iteration = 0; iteration < iterations; ++iteration) {
        std::fill(u.begin(), u.end(), 0.0);
        for (size_t i = 0; i < rows; ++i)
            for (size_t j = 0; j < cols; ++j)
                u[i] += std::abs(at(matrix, i, j)) * v[j];
        double norm = std::sqrt(std::inner_product(u.begin(), u.end(), u.begin(), 0.0));
        if (norm == 0.0) return Tensor(matrix.shape(), 0.0);
        for (double& value : u) value /= norm;
        std::fill(v.begin(), v.end(), 0.0);
        for (size_t j = 0; j < cols; ++j)
            for (size_t i = 0; i < rows; ++i)
                v[j] += std::abs(at(matrix, i, j)) * u[i];
        norm = std::sqrt(std::inner_product(v.begin(), v.end(), v.begin(), 0.0));
        if (norm == 0.0) return Tensor(matrix.shape(), 0.0);
        for (double& value : v) value /= norm;
    }
    double sigma = 0.0;
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            sigma += u[i] * std::abs(at(matrix, i, j)) * v[j];
    Tensor result(matrix.shape(), 0.0);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            at(result, i, j) = (at(matrix, i, j) < 0.0 ? -1.0 : 1.0) * sigma * u[i] * v[j];
    return result;
}

Tensor solve(const Tensor& coefficients, const Tensor& rhs) {
    validate_matrix(coefficients, "solve");
    validate_matrix(rhs, "solve");
    const size_t n = coefficients.shape()[0];
    if (coefficients.shape()[1] != n || rhs.shape()[0] != n)
        throw std::invalid_argument("solve: incompatible dimensions");
    const size_t columns = rhs.shape()[1];
    Tensor a = coefficients.clone();
    Tensor b = rhs.clone();
    for (size_t pivot = 0; pivot < n; ++pivot) {
        size_t best = pivot;
        for (size_t row = pivot + 1; row < n; ++row)
            if (std::abs(at(a, row, pivot)) > std::abs(at(a, best, pivot))) best = row;
        if (std::abs(at(a, best, pivot)) < 1e-15) throw std::runtime_error("solve: singular matrix");
        if (best != pivot) {
            for (size_t col = 0; col < n; ++col) std::swap(at(a, pivot, col), at(a, best, col));
            for (size_t col = 0; col < columns; ++col) std::swap(at(b, pivot, col), at(b, best, col));
        }
        const double divisor = at(a, pivot, pivot);
        for (size_t col = pivot; col < n; ++col) at(a, pivot, col) /= divisor;
        for (size_t col = 0; col < columns; ++col) at(b, pivot, col) /= divisor;
        for (size_t row = 0; row < n; ++row) {
            if (row == pivot) continue;
            const double factor = at(a, row, pivot);
            for (size_t col = pivot; col < n; ++col) at(a, row, col) -= factor * at(a, pivot, col);
            for (size_t col = 0; col < columns; ++col) at(b, row, col) -= factor * at(b, pivot, col);
        }
    }
    return b;
}

double squared_error(const Tensor& a, const Tensor& b) {
    if (a.shape() != b.shape()) throw std::invalid_argument("squared_error: shape mismatch");
    double result = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        const double difference = a.data()[i] - b.data()[i];
        result += difference * difference;
    }
    return result;
}

} // namespace samsung_internal
} // namespace scaling
} // namespace deep_learning
} // namespace ml
