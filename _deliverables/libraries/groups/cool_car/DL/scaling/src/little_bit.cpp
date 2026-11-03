#include "little_bit.h"

#include "samsung_quantization_internal.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

using samsung_internal::truncated_svd;
using samsung_internal::validate_matrix;

double matrix_value(const Tensor& x, size_t row, size_t col) {
    return x.data()[row * x.shape()[1] + col];
}

std::pair<Tensor, Tensor> rank_one_scales(const Tensor& matrix) {
    Tensor magnitude(matrix.shape(), 0.0);
    for (size_t i = 0; i < matrix.size(); ++i) magnitude.data()[i] = std::abs(matrix.data()[i]);
    const auto decomposition = truncated_svd(magnitude, 1);
    const double root = std::sqrt(decomposition.singular_values[0]);
    Tensor rows({matrix.shape()[0]}, 0.0);
    Tensor columns({matrix.shape()[1]}, 0.0);
    for (size_t i = 0; i < rows.size(); ++i)
        rows.data()[i] = std::abs(matrix_value(decomposition.u, i, 0)) * root;
    for (size_t j = 0; j < columns.size(); ++j)
        columns.data()[j] = std::abs(matrix_value(decomposition.vt, 0, j)) * root;
    return {rows, columns};
}

LittleBitFactors factors_from_balanced(const Tensor& u, const Tensor& v) {
    auto u_scales = rank_one_scales(u);
    auto v_scales = rank_one_scales(v);
    Tensor latent({u.shape()[1]}, 0.0);
    for (size_t i = 0; i < latent.size(); ++i)
        latent.data()[i] = u_scales.second.data()[i] * v_scales.first.data()[i];
    return {
        pack_binary_signs(u),
        pack_binary_signs(v),
        std::move(u_scales.first),
        std::move(latent),
        std::move(v_scales.second)
    };
}

std::pair<Tensor, Tensor> balanced_svd(const Tensor& weights, size_t rank) {
    const auto decomposition = truncated_svd(weights, rank);
    Tensor u({weights.shape()[0], rank}, 0.0);
    Tensor v({rank, weights.shape()[1]}, 0.0);
    for (size_t k = 0; k < rank; ++k) {
        const double root = std::sqrt(decomposition.singular_values[k]);
        for (size_t i = 0; i < weights.shape()[0]; ++i)
            u.data()[i * rank + k] = decomposition.u.data()[i * rank + k] * root;
        for (size_t j = 0; j < weights.shape()[1]; ++j)
            v.data()[k * weights.shape()[1] + j] =
                decomposition.vt.data()[k * weights.shape()[1] + j] * root;
    }
    return {u, v};
}

} // namespace

int PackedBinaryMatrix::value(size_t row, size_t col) const {
    if (row >= rows || col >= cols || words.size() != rows * words_per_row())
        throw std::out_of_range("PackedBinaryMatrix::value: invalid index or payload");
    const uint32_t bit = (words[row * words_per_row() + col / 32] >> (col % 32)) & 1u;
    return bit == 0 ? 1 : -1;
}

PackedBinaryMatrix pack_binary_signs(const Tensor& matrix) {
    validate_matrix(matrix, "pack_binary_signs");
    PackedBinaryMatrix result;
    result.rows = matrix.shape()[0];
    result.cols = matrix.shape()[1];
    result.words.assign(result.rows * result.words_per_row(), 0u);
    for (size_t row = 0; row < result.rows; ++row) {
        for (size_t col = 0; col < result.cols; ++col) {
            if (matrix.data()[row * result.cols + col] < 0.0)
                result.words[row * result.words_per_row() + col / 32] |= uint32_t{1} << (col % 32);
        }
    }
    return result;
}

Tensor unpack_binary_signs(const PackedBinaryMatrix& matrix) {
    if (matrix.rows == 0 || matrix.cols == 0 ||
        matrix.words.size() != matrix.rows * matrix.words_per_row())
        throw std::invalid_argument("unpack_binary_signs: malformed packed matrix");
    Tensor result({matrix.rows, matrix.cols}, 0.0);
    for (size_t row = 0; row < matrix.rows; ++row)
        for (size_t col = 0; col < matrix.cols; ++col)
            result.data()[row * matrix.cols + col] = matrix.value(row, col);
    return result;
}

size_t LittleBitFactors::logical_storage_bits() const {
    return u_sign.logical_bits() + v_sign.logical_bits() +
           16 * (h.size() + latent.size() + g.size());
}

size_t LittleBitFactors::packed_storage_bits() const {
    return u_sign.packed_bits() + v_sign.packed_bits() +
           16 * (h.size() + latent.size() + g.size());
}

double LittleBitFactors::bits_per_weight() const {
    const size_t count = u_sign.rows * v_sign.cols;
    return count == 0 ? 0.0 :
        static_cast<double>(logical_storage_bits()) / static_cast<double>(count);
}

Tensor LittleBitFactors::reconstruct() const {
    if (u_sign.cols != v_sign.rows || h.shape() != std::vector<size_t>{u_sign.rows} ||
        latent.shape() != std::vector<size_t>{u_sign.cols} ||
        g.shape() != std::vector<size_t>{v_sign.cols})
        throw std::invalid_argument("LittleBitFactors::reconstruct: inconsistent factor shapes");
    Tensor result({u_sign.rows, v_sign.cols}, 0.0);
    for (size_t out = 0; out < u_sign.rows; ++out) {
        for (size_t in = 0; in < v_sign.cols; ++in) {
            double sum = 0.0;
            for (size_t k = 0; k < u_sign.cols; ++k)
                sum += u_sign.value(out, k) * latent.data()[k] * v_sign.value(k, in);
            result.data()[out * v_sign.cols + in] = h.data()[out] * sum * g.data()[in];
        }
    }
    return result;
}

Tensor LittleBitFactors::forward(const Tensor& input) const {
    validate_matrix(input, "LittleBitFactors::forward");
    if (input.shape()[1] != v_sign.cols)
        throw std::invalid_argument("LittleBitFactors::forward: input feature mismatch");
    return input.matmul(reconstruct().transpose());
}

size_t LittleBitResult::logical_storage_bits() const {
    return primary.logical_storage_bits() + (has_residual ? residual.logical_storage_bits() : 0);
}
size_t LittleBitResult::packed_storage_bits() const {
    return primary.packed_storage_bits() + (has_residual ? residual.packed_storage_bits() : 0);
}
double LittleBitResult::bits_per_weight() const {
    const size_t count = primary.u_sign.rows * primary.v_sign.cols;
    return count == 0 ? 0.0 :
        static_cast<double>(logical_storage_bits()) / static_cast<double>(count);
}
Tensor LittleBitResult::reconstruct() const {
    Tensor result = primary.reconstruct();
    if (has_residual) result += residual.reconstruct();
    return result;
}
Tensor LittleBitResult::forward(const Tensor& input) const {
    Tensor result = primary.forward(input);
    if (has_residual) result += residual.forward(input);
    return result;
}

LittleBitResult little_bit_quantize(const Tensor& weights, const LittleBitConfig& config) {
    validate_matrix(weights, "little_bit_quantize");
    if (config.rank == 0 || config.rank > std::min(weights.shape()[0], weights.shape()[1]))
        throw std::invalid_argument("little_bit_quantize: invalid rank");
    auto balanced = balanced_svd(weights, config.rank);
    LittleBitResult result;
    result.primary = factors_from_balanced(balanced.first, balanced.second);
    if (config.residual_compensation) {
        Tensor remaining = weights - result.primary.reconstruct();
        auto residual_balanced = balanced_svd(remaining, config.rank);
        result.residual = factors_from_balanced(residual_balanced.first, residual_balanced.second);
        result.has_residual = true;
    }
    return result;
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
