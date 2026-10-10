#ifndef ML_DEEP_LEARNING_SCALING_DOMINO_H
#define ML_DEEP_LEARNING_SCALING_DOMINO_H

#include "tensor.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// @brief Generic tensor slicing with communication/computation overlap.
///
/// Implements the row-wise (input) and column-wise (weight) slicing and the
/// fine-grained overlap scheduling of Wang et al., "Domino: Eliminating
/// Communication in LLM Training via Generic Tensor Slicing and Overlapping"
/// (arXiv:2409.15241), citation key @c wang_domino_2024.
struct DominoConfig {
    /// Number of independent row (batch/sequence) slices per layer.
    size_t row_chunks = 2;
    /// Number of column slices of the weight matrix, used for the second GEMM
    /// of an MLP block where the output requires an all-reduce.
    size_t column_chunks = 2;
    bool overlap = true;
};

/// One entry of the generated pipeline: the compute of chunk i is issued while
/// the collective of chunk i-1 is still in flight.
struct OverlapStep {
    size_t layer = 0;
    size_t chunk = 0;
    std::string compute_tag;
    /// Chunk whose collective is hidden behind this compute; npos when none.
    size_t overlapped_comm_chunk = 0;
    bool has_overlapped_comm = false;
};

class DominoScheduler {
public:
    explicit DominoScheduler(DominoConfig config);

    const DominoConfig& config() const { return config_; }

    /// Builds the interleaved compute/communicate schedule for @p num_layers.
    std::vector<OverlapStep> build_schedule(size_t num_layers) const;

    /// Fraction of the collective time that the schedule manages to hide.
    double hidden_communication_fraction() const;

    /// Iteration time given per-layer compute and collective costs. Without
    /// slicing the two costs are serial; with slicing all but the final chunk's
    /// collective disappears behind compute.
    double estimated_iteration_seconds(double compute_seconds,
                                       double communication_seconds,
                                       size_t num_layers) const;

    /// Speedup over the non-overlapped baseline.
    double speedup(double compute_seconds, double communication_seconds, size_t num_layers) const;

    static std::vector<Tensor> split_rows(const Tensor& tensor, size_t chunks);
    static std::vector<Tensor> split_columns(const Tensor& tensor, size_t chunks);
    static Tensor concat_rows(const std::vector<Tensor>& chunks);
    static Tensor concat_columns(const std::vector<Tensor>& chunks);

    /// Runs `input * weight` as a sliced pipeline. @p all_reduce is invoked once
    /// per row chunk, standing in for the tensor-parallel collective that Domino
    /// overlaps with the next chunk's GEMM. The mathematical result is identical
    /// to the unsliced product.
    Tensor forward_sliced(const Tensor& input,
                          const Tensor& weight,
                          const std::function<void(Tensor&, size_t)>& all_reduce) const;

private:
    DominoConfig config_;
};

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_DOMINO_H
