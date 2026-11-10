#ifndef ML_DEEP_LEARNING_SCALING_LITTLE_BIT2_H
#define ML_DEEP_LEARNING_SCALING_LITTLE_BIT2_H

#include "little_bit.h"

#include <cstddef>
#include <cstdint>

namespace ml {
namespace deep_learning {
namespace scaling {

struct LittleBit2Config {
    size_t rank = 1;
    bool residual_compensation = false;
    size_t alignment_iterations = 50;
    uint32_t seed = 0;
};

/// CPU reference for LittleBit-2 (Lee et al., ICML 2026,
/// arXiv:2603.00042). After balanced SVD, Joint-ITQ solves the alternating
/// binary assignment/orthogonal-Procrustes problem on [U; V^T]. Rotation is
/// internal, so the deployed format and bit cost are identical to LittleBit.
LittleBitResult little_bit2_quantize(
    const Tensor& weights,
    const LittleBit2Config& config = LittleBit2Config());

/// Computes the orthogonal Joint-ITQ rotation for [samples, latent_rank].
Tensor joint_itq_rotation(const Tensor& latent_rows, size_t iterations = 50,
                          uint32_t seed = 0);

/// Joint-ITQ objective ||sign(XR)-XR||_F^2. Exposed for deterministic
/// initialization validation; @p latent_rows and @p rotation must be 2-D.
double latent_alignment_error(const Tensor& latent_rows, const Tensor& rotation);

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif
