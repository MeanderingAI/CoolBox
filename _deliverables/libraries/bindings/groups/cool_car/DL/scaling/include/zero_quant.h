#ifndef ML_DEEP_LEARNING_SCALING_ZERO_QUANT_H
#define ML_DEEP_LEARNING_SCALING_ZERO_QUANT_H

#include "tensor.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// @brief Post-training quantization for transformer weights and activations.
///
/// Implements the group-wise weight quantization, token-wise activation
/// quantization and layer-by-layer knowledge distillation (LKD) of
/// Yao et al., "ZeroQuant: Efficient and Affordable Post-Training Quantization
/// for Large-Scale Transformers", NeurIPS 2022 (arXiv:2206.01861), citation key
/// @c yao_zeroquant_2022.
struct QuantizedTensor {
    std::vector<int32_t> values;
    std::vector<double> scales;        ///< One symmetric scale per group.
    std::vector<size_t> shape;
    size_t group_size = 0;             ///< Elements sharing a scale.
    int bits = 8;

    size_t num_groups() const { return scales.size(); }
    /// Bytes needed for the quantized payload plus its scales.
    double storage_bytes() const;
};

/// Symmetric group-wise quantization: the weight matrix is split into groups of
/// @p group_size contiguous elements, each with its own scale, which keeps INT4
/// accurate enough to be used without retraining.
QuantizedTensor group_wise_quantize(const Tensor& weights, size_t group_size, int bits);

/// Dynamic per-token (per-row) activation quantization. The scale is computed at
/// run time from the row's own range, so no calibration set is needed.
QuantizedTensor token_wise_quantize(const Tensor& activations, int bits);

Tensor dequantize(const QuantizedTensor& quantized);

/// Round-trip a tensor through quantization, i.e. simulated quantization.
Tensor fake_quantize(const Tensor& tensor, size_t group_size, int bits);

/// Mean squared error between a tensor and its reconstruction.
double quantization_error(const Tensor& original, const Tensor& reconstructed);

/// Memory footprint of @p quantized relative to the fp16 original.
double memory_reduction(const QuantizedTensor& quantized);

/// Per-layer mixed precision recipe: ZeroQuant keeps attention weights at INT8
/// and pushes the fully connected weights to INT4.
struct QuantizationRecipe {
    int attention_weight_bits = 8;
    int mlp_weight_bits = 4;
    int activation_bits = 8;
    size_t weight_group_size = 64;
};

/// Layer-by-layer knowledge distillation.
///
/// Optimizes the floating point weight so that its quantized version reproduces
/// the teacher layer's output on the given inputs. Only one layer's activations
/// are needed at a time, so a whole model can be distilled without holding the
/// teacher and student in memory simultaneously and without the original
/// training data.
class LayerwiseKnowledgeDistiller {
public:
    struct Config {
        size_t iterations = 25;
        double learning_rate = 1e-2;
        size_t group_size = 64;
        int bits = 4;
    };

    LayerwiseKnowledgeDistiller();
    explicit LayerwiseKnowledgeDistiller(Config config);

    /// @param teacher_weight [in, out] fp weight of the layer being quantized.
    /// @param calibration_inputs Activations captured at this layer's input.
    /// @return The distilled fp weight; quantize it to obtain the student.
    Tensor distill(const Tensor& teacher_weight,
                   const std::vector<Tensor>& calibration_inputs) const;

    /// Distillation loss trace, one entry per iteration.
    const std::vector<double>& loss_history() const { return loss_history_; }

private:
    Config config_;
    mutable std::vector<double> loss_history_;
};

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_ZERO_QUANT_H
