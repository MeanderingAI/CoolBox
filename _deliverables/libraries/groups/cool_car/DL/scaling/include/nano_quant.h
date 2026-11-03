#ifndef ML_DEEP_LEARNING_SCALING_NANO_QUANT_H
#define ML_DEEP_LEARNING_SCALING_NANO_QUANT_H

#include "little_bit.h"

#include <cstddef>
#include <cstdint>

namespace ml {
namespace deep_learning {
namespace scaling {

struct NanoQuantConfig {
    size_t rank = 1;
    size_t outer_iterations = 400;
    size_t projection_iterations = 5;
    double regularization = 3e-2;
    double epsilon = 1e-12;
    uint32_t seed = 0;
};

/// Scale-Binary-Binary-Scale representation used by NanoQuant:
/// diag(post) U_sign V_sign diag(pre).
struct NanoQuantFactors {
    PackedBinaryMatrix u_sign; ///< [out_features, rank]
    PackedBinaryMatrix v_sign; ///< [rank, in_features]
    Tensor pre_scale;          ///< [in_features]
    Tensor post_scale;         ///< [out_features]

    size_t logical_storage_bits() const;
    size_t packed_storage_bits() const;
    double bits_per_weight() const;
    Tensor reconstruct() const;
    Tensor forward(const Tensor& input) const;
};

/// Deterministic CPU reference for NanoQuant (Chong et al., 2026,
/// arXiv:2602.06694). This is the paper's layer-local ADMM factorization and
/// rank-one-magnitude projection. Full-model calibration/tuning is intentionally
/// outside this reference primitive.
///
/// Optional importance tensors contain non-negative squared input/output norms
/// with shapes [in_features] and [out_features]. Empty tensors mean all ones.
NanoQuantFactors nano_quantize(const Tensor& weights,
                               const NanoQuantConfig& config = NanoQuantConfig(),
                               const Tensor& input_importance = Tensor(),
                               const Tensor& output_importance = Tensor());

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif
