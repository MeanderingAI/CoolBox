#include "universal_checkpoint.h"

#include "collective.h"

#include <functional>
#include <numeric>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

size_t element_count(const std::vector<size_t>& shape) {
    return std::accumulate(shape.begin(), shape.end(), static_cast<size_t>(1),
                           std::multiplies<size_t>());
}

const std::vector<double>& fetch(const std::vector<RankState>& states,
                                 size_t rank,
                                 const std::string& name) {
    for (const auto& state : states) {
        if (state.rank != rank) {
            continue;
        }
        const auto found = state.tensors.find(name);
        if (found == state.tensors.end()) {
            break;
        }
        return found->second;
    }
    throw std::runtime_error("UniversalCheckpoint: missing tensor '" + name + "' on rank " +
                             std::to_string(rank));
}

size_t row_stride(const std::vector<size_t>& shape) {
    if (shape.size() < 2) {
        return 1;
    }
    return element_count(std::vector<size_t>(shape.begin() + 1, shape.end()));
}

} // namespace

std::string to_string(ParamPattern pattern) {
    switch (pattern) {
        case ParamPattern::kUnique: return "unique";
        case ParamPattern::kReplicated: return "replicated";
        case ParamPattern::kRowParallel: return "row_parallel";
        case ParamPattern::kColumnParallel: return "column_parallel";
        case ParamPattern::kFlattenedShard: return "flattened_shard";
    }
    return "unknown";
}

size_t ParallelConfig::rank_of(size_t pipeline_stage, size_t data_index, size_t tensor_index) const {
    if (pipeline_stage >= pipeline_parallel || data_index >= data_parallel ||
        tensor_index >= tensor_parallel) {
        throw std::out_of_range("ParallelConfig::rank_of: coordinate is out of range");
    }
    return pipeline_stage * (data_parallel * tensor_parallel) + data_index * tensor_parallel +
           tensor_index;
}

UniversalCheckpoint UniversalCheckpoint::consolidate(const std::vector<ParameterSpec>& specs,
                                                     const ParallelConfig& source,
                                                     const std::vector<RankState>& rank_states) {
    UniversalCheckpoint checkpoint;
    checkpoint.specs_ = specs;

    for (const ParameterSpec& spec : specs) {
        const size_t total = element_count(spec.shape);
        std::vector<double> atom;
        atom.reserve(total);

        switch (spec.pattern) {
            case ParamPattern::kUnique:
            case ParamPattern::kReplicated: {
                // Every holder has the same bytes; take the canonical replica.
                atom = fetch(rank_states, source.rank_of(spec.pipeline_stage, 0, 0), spec.name);
                break;
            }
            case ParamPattern::kRowParallel: {
                for (size_t tensor = 0; tensor < source.tensor_parallel; ++tensor) {
                    const auto& shard =
                        fetch(rank_states, source.rank_of(spec.pipeline_stage, 0, tensor), spec.name);
                    atom.insert(atom.end(), shard.begin(), shard.end());
                }
                break;
            }
            case ParamPattern::kColumnParallel: {
                if (spec.shape.size() != 2) {
                    throw std::invalid_argument(
                        "UniversalCheckpoint: column-parallel parameters must be 2D");
                }
                const size_t rows = spec.shape[0];
                const size_t cols = spec.shape[1];
                const auto column_ranges = partition_evenly(cols, source.tensor_parallel);

                atom.assign(total, 0.0);
                for (size_t tensor = 0; tensor < source.tensor_parallel; ++tensor) {
                    const auto& shard =
                        fetch(rank_states, source.rank_of(spec.pipeline_stage, 0, tensor), spec.name);
                    const ShardRange& range = column_ranges[tensor];
                    for (size_t r = 0; r < rows; ++r) {
                        for (size_t c = 0; c < range.count; ++c) {
                            atom[r * cols + range.offset + c] = shard[r * range.count + c];
                        }
                    }
                }
                break;
            }
            case ParamPattern::kFlattenedShard: {
                for (size_t data = 0; data < source.data_parallel; ++data) {
                    const auto& shard =
                        fetch(rank_states, source.rank_of(spec.pipeline_stage, data, 0), spec.name);
                    atom.insert(atom.end(), shard.begin(), shard.end());
                }
                break;
            }
        }

        if (atom.size() != total) {
            throw std::runtime_error("UniversalCheckpoint: consolidated size mismatch for '" +
                                     spec.name + "'");
        }
        checkpoint.atoms_[spec.name] = std::move(atom);
    }

    return checkpoint;
}

std::vector<RankState> UniversalCheckpoint::reconfigure(const ParallelConfig& target) const {
    std::vector<RankState> states(target.world_size());
    for (size_t rank = 0; rank < states.size(); ++rank) {
        states[rank].rank = rank;
    }

    for (const ParameterSpec& spec : specs_) {
        if (spec.pipeline_stage >= target.pipeline_parallel) {
            throw std::invalid_argument(
                "UniversalCheckpoint::reconfigure: target has fewer pipeline stages than the model");
        }
        const std::vector<double>& atom = this->atom(spec.name);

        switch (spec.pattern) {
            case ParamPattern::kUnique: {
                states[target.rank_of(spec.pipeline_stage, 0, 0)].tensors[spec.name] = atom;
                break;
            }
            case ParamPattern::kReplicated: {
                for (size_t data = 0; data < target.data_parallel; ++data) {
                    for (size_t tensor = 0; tensor < target.tensor_parallel; ++tensor) {
                        states[target.rank_of(spec.pipeline_stage, data, tensor)]
                            .tensors[spec.name] = atom;
                    }
                }
                break;
            }
            case ParamPattern::kRowParallel: {
                const size_t stride = row_stride(spec.shape);
                const auto row_ranges = partition_evenly(spec.shape.front(), target.tensor_parallel);
                for (size_t tensor = 0; tensor < target.tensor_parallel; ++tensor) {
                    const ShardRange& range = row_ranges[tensor];
                    const std::vector<double> shard(
                        atom.begin() + static_cast<std::ptrdiff_t>(range.offset * stride),
                        atom.begin() + static_cast<std::ptrdiff_t>(range.end() * stride));
                    for (size_t data = 0; data < target.data_parallel; ++data) {
                        states[target.rank_of(spec.pipeline_stage, data, tensor)]
                            .tensors[spec.name] = shard;
                    }
                }
                break;
            }
            case ParamPattern::kColumnParallel: {
                const size_t rows = spec.shape[0];
                const size_t cols = spec.shape[1];
                const auto column_ranges = partition_evenly(cols, target.tensor_parallel);
                for (size_t tensor = 0; tensor < target.tensor_parallel; ++tensor) {
                    const ShardRange& range = column_ranges[tensor];
                    std::vector<double> shard(rows * range.count, 0.0);
                    for (size_t r = 0; r < rows; ++r) {
                        for (size_t c = 0; c < range.count; ++c) {
                            shard[r * range.count + c] = atom[r * cols + range.offset + c];
                        }
                    }
                    for (size_t data = 0; data < target.data_parallel; ++data) {
                        states[target.rank_of(spec.pipeline_stage, data, tensor)]
                            .tensors[spec.name] = shard;
                    }
                }
                break;
            }
            case ParamPattern::kFlattenedShard: {
                const auto ranges = partition_evenly(atom.size(), target.data_parallel);
                for (size_t data = 0; data < target.data_parallel; ++data) {
                    const ShardRange& range = ranges[data];
                    const std::vector<double> shard(
                        atom.begin() + static_cast<std::ptrdiff_t>(range.offset),
                        atom.begin() + static_cast<std::ptrdiff_t>(range.end()));
                    for (size_t tensor = 0; tensor < target.tensor_parallel; ++tensor) {
                        states[target.rank_of(spec.pipeline_stage, data, tensor)]
                            .tensors[spec.name] = shard;
                    }
                }
                break;
            }
        }
    }

    return states;
}

const std::vector<double>& UniversalCheckpoint::atom(const std::string& name) const {
    const auto found = atoms_.find(name);
    if (found == atoms_.end()) {
        throw std::out_of_range("UniversalCheckpoint: unknown atom '" + name + "'");
    }
    return found->second;
}

bool UniversalCheckpoint::contains(const std::string& name) const {
    return atoms_.find(name) != atoms_.end();
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
