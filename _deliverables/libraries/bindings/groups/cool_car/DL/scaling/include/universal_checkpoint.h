#ifndef ML_DEEP_LEARNING_SCALING_UNIVERSAL_CHECKPOINT_H
#define ML_DEEP_LEARNING_SCALING_UNIVERSAL_CHECKPOINT_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// @brief Parallelism-agnostic checkpoints with pattern-based reconfiguration.
///
/// Implements the atom checkpoint format and the pattern-based reconfiguration
/// pipeline of Lian et al., "Universal Checkpointing: A Flexible and Efficient
/// Distributed Checkpointing System for Large-Scale DNN Training with
/// Reconfigurable Parallelism" (arXiv:2406.18820), citation key
/// @c lian_universal_checkpointing_2024.

/// How a parameter is distributed over a parallel training job.
enum class ParamPattern {
    kUnique,          ///< Lives on a single rank, e.g. an embedding on stage 0.
    kReplicated,      ///< Identical on every tensor-parallel rank, e.g. LayerNorm.
    kRowParallel,     ///< Sharded along dimension 0 across tensor-parallel ranks.
    kColumnParallel,  ///< Sharded along dimension 1 across tensor-parallel ranks.
    kFlattenedShard   ///< ZeRO-style flat shard across data-parallel ranks.
};

std::string to_string(ParamPattern pattern);

struct ParallelConfig {
    size_t data_parallel = 1;
    size_t tensor_parallel = 1;
    size_t pipeline_parallel = 1;

    size_t world_size() const { return data_parallel * tensor_parallel * pipeline_parallel; }
    /// Linear rank index for the (pipeline, data, tensor) coordinate.
    size_t rank_of(size_t pipeline_stage, size_t data_index, size_t tensor_index) const;
};

struct ParameterSpec {
    std::string name;
    std::vector<size_t> shape;                    ///< Full, unsharded shape.
    ParamPattern pattern = ParamPattern::kReplicated;
    size_t pipeline_stage = 0;
};

/// The tensors a single rank holds on disk, keyed by parameter name.
struct RankState {
    size_t rank = 0;
    std::map<std::string, std::vector<double>> tensors;
};

/// A consolidated, parallelism-independent checkpoint.
class UniversalCheckpoint {
public:
    /// Merges the per-rank shards produced under @p source into whole atoms.
    static UniversalCheckpoint consolidate(const std::vector<ParameterSpec>& specs,
                                           const ParallelConfig& source,
                                           const std::vector<RankState>& rank_states);

    /// Splits the atoms back out for an arbitrary target parallelism.
    std::vector<RankState> reconfigure(const ParallelConfig& target) const;

    const std::vector<ParameterSpec>& specs() const { return specs_; }
    const std::vector<double>& atom(const std::string& name) const;
    bool contains(const std::string& name) const;
    size_t atom_count() const { return atoms_.size(); }

private:
    std::vector<ParameterSpec> specs_;
    std::map<std::string, std::vector<double>> atoms_;
};

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_UNIVERSAL_CHECKPOINT_H
