#ifndef ML_DEEP_LEARNING_SCALING_ONE_BIT_LAMB_H
#define ML_DEEP_LEARNING_SCALING_ONE_BIT_LAMB_H

#include "collective.h"
#include "optimizer.h"
#include "tensor.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// @brief Error-compensated 1-bit momentum compression and the 1-bit LAMB optimizer.
///
/// Implements Tang et al., "1-bit LAMB: Communication Efficient Large-Scale
/// Large-Batch Training with LAMB's Convergence Speed", HiPC 2022
/// (arXiv:2104.06069), citation key @c tang_one_bit_lamb_2021.

/// One chunk-quantized buffer: a sign bit per element plus one magnitude per chunk.
struct OneBitPayload {
    std::vector<uint8_t> signs;   ///< Bit-packed, 8 elements per byte, 1 = positive.
    std::vector<double> scales;   ///< One magnitude per chunk.
    size_t chunk_size = 0;
    size_t element_count = 0;
};

/// Compresses @p values to 1 bit per element after adding the residual @p error,
/// then overwrites @p error with the new residual. @p error is resized on first use.
OneBitPayload compress_one_bit(const std::vector<double>& values,
                               std::vector<double>& error,
                               size_t chunk_size = 512);

std::vector<double> decompress_one_bit(const OneBitPayload& payload);

/// Compressed bytes divided by uncompressed fp32 bytes.
double compression_ratio(const OneBitPayload& payload);

/// Error-compensated all-reduce: every rank compresses its own buffer, the
/// decompressed signals are summed, and each rank keeps its residual for the
/// next step. Returns the reduced buffer.
std::vector<double> compressed_all_reduce(CollectiveGroup& group,
                                          const std::vector<std::vector<double>>& per_rank_values,
                                          std::vector<std::vector<double>>& per_rank_errors,
                                          size_t chunk_size = 512);

/// LAMB with a warmup phase followed by 1-bit compressed momentum.
///
/// During warmup the optimizer runs vanilla LAMB and accumulates the variance
/// term. At the end of warmup the variance and the resulting update-scaling
/// coefficient are frozen, which is what makes the compression phase safe: the
/// adaptive term no longer changes, so only the momentum needs communicating.
class OneBitLamb : public Optimizer {
public:
    struct Config {
        double learning_rate = 1e-3;
        double beta1 = 0.9;
        double beta2 = 0.999;
        double epsilon = 1e-6;
        double weight_decay = 0.0;
        size_t warmup_steps = 100;
        double min_coeff = 0.01;   ///< Lower clamp on the LAMB trust ratio.
        double max_coeff = 10.0;   ///< Upper clamp on the LAMB trust ratio.
        size_t chunk_size = 512;
        /// Re-freezes the variance term every N steps; 0 disables refreshing.
        size_t coeff_refresh_interval = 0;
    };

    OneBitLamb();
    explicit OneBitLamb(Config config);

    void step(Tensor& parameters, const Tensor& gradients) override;
    std::string name() const override { return "OneBitLAMB"; }
    void reset() override;

    bool in_compression_phase() const { return step_count_ > config_.warmup_steps; }
    size_t step_count() const { return step_count_; }
    double last_trust_ratio() const { return last_trust_ratio_; }
    /// Communication volume of the last step relative to uncompressed fp32.
    double last_compression_ratio() const { return last_compression_ratio_; }
    const std::vector<double>& error_compensation() const { return error_; }
    /// Variance snapshot taken when the warmup phase ended.
    const std::vector<double>& frozen_variance() const { return frozen_variance_; }

private:
    void ensure_initialized(size_t elements);

    Config config_;
    std::vector<double> momentum_;
    std::vector<double> variance_;
    std::vector<double> frozen_variance_;
    std::vector<double> error_;
    size_t step_count_ = 0;
    double last_trust_ratio_ = 1.0;
    double last_compression_ratio_ = 1.0;
    bool initialized_ = false;
};

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_ONE_BIT_LAMB_H
