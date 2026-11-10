#ifndef ML_DEEP_LEARNING_SCALING_LITTLE_BIT_H
#define ML_DEEP_LEARNING_SCALING_LITTLE_BIT_H

#include "../../layers/include/tensor.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// Row-major, LSB-first storage for a matrix whose entries are exactly -1/+1.
/// A zero input is mapped to +1, matching Samsung's reference implementation.
struct PackedBinaryMatrix {
    size_t rows = 0;
    size_t cols = 0;
    std::vector<uint32_t> words;

    size_t words_per_row() const { return (cols + 31) / 32; }
    size_t logical_bits() const { return rows * cols; }
    size_t packed_bits() const { return words.size() * 32; }
    int value(size_t row, size_t col) const;
};

PackedBinaryMatrix pack_binary_signs(const Tensor& matrix);
Tensor unpack_binary_signs(const PackedBinaryMatrix& matrix);

/// LittleBit's deployed low-rank binary representation:
/// diag(h) sign(U) diag(l) sign(V) diag(g).
///
/// Scale storage is accounted as FP16, as in the paper and official code,
/// although Tensor uses double in this portable CPU reference.
struct LittleBitFactors {
    PackedBinaryMatrix u_sign; ///< [out_features, rank]
    PackedBinaryMatrix v_sign; ///< [rank, in_features]
    Tensor h;                  ///< [out_features]
    Tensor latent;             ///< [rank]
    Tensor g;                  ///< [in_features]

    size_t logical_storage_bits() const;
    size_t packed_storage_bits() const;
    double bits_per_weight() const;
    Tensor reconstruct() const;
    Tensor forward(const Tensor& input) const;
};

struct LittleBitConfig {
    size_t rank = 1;
    bool residual_compensation = false;
};

/// CPU reference for LittleBit (Lee et al., NeurIPS 2025, arXiv:2506.13771).
///
/// It performs balanced truncated SVD (Dual-SVID), binarizes both factors,
/// and obtains h/l/g from rank-one approximations of their magnitudes.
/// Optional residual compensation applies the same procedure to W-W_primary.
struct LittleBitResult {
    LittleBitFactors primary;
    bool has_residual = false;
    LittleBitFactors residual;

    size_t logical_storage_bits() const;
    size_t packed_storage_bits() const;
    double bits_per_weight() const;
    Tensor reconstruct() const;
    Tensor forward(const Tensor& input) const;
};

LittleBitResult little_bit_quantize(const Tensor& weights,
                                    const LittleBitConfig& config = LittleBitConfig());

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif
