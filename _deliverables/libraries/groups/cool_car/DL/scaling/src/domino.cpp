#include "domino.h"

#include "collective.h"

#include <algorithm>
#include <stdexcept>

namespace ml {
namespace deep_learning {
namespace scaling {

DominoScheduler::DominoScheduler(DominoConfig config) : config_(config) {
    if (config_.row_chunks == 0 || config_.column_chunks == 0) {
        throw std::invalid_argument("DominoScheduler: chunk counts must be > 0");
    }
}

std::vector<OverlapStep> DominoScheduler::build_schedule(size_t num_layers) const {
    std::vector<OverlapStep> schedule;
    schedule.reserve(num_layers * config_.row_chunks);

    for (size_t layer = 0; layer < num_layers; ++layer) {
        for (size_t chunk = 0; chunk < config_.row_chunks; ++chunk) {
            OverlapStep step;
            step.layer = layer;
            step.chunk = chunk;
            step.compute_tag = "layer" + std::to_string(layer) + ".chunk" + std::to_string(chunk);
            // Chunk i's GEMM hides the collective issued for chunk i-1.
            if (config_.overlap && chunk > 0) {
                step.overlapped_comm_chunk = chunk - 1;
                step.has_overlapped_comm = true;
            }
            schedule.push_back(std::move(step));
        }
    }
    return schedule;
}

double DominoScheduler::hidden_communication_fraction() const {
    if (!config_.overlap) {
        return 0.0;
    }
    // Every chunk but the last has its collective covered by the next GEMM.
    return static_cast<double>(config_.row_chunks - 1) / static_cast<double>(config_.row_chunks);
}

double DominoScheduler::estimated_iteration_seconds(double compute_seconds,
                                                    double communication_seconds,
                                                    size_t num_layers) const {
    const double exposed = communication_seconds * (1.0 - hidden_communication_fraction());
    return static_cast<double>(num_layers) * (compute_seconds + exposed);
}

double DominoScheduler::speedup(double compute_seconds,
                                double communication_seconds,
                                size_t num_layers) const {
    const double baseline = static_cast<double>(num_layers) * (compute_seconds + communication_seconds);
    const double sliced = estimated_iteration_seconds(compute_seconds, communication_seconds, num_layers);
    return sliced > 0.0 ? baseline / sliced : 0.0;
}

std::vector<Tensor> DominoScheduler::split_rows(const Tensor& tensor, size_t chunks) {
    if (tensor.shape().size() != 2) {
        throw std::invalid_argument("DominoScheduler::split_rows: tensor must be 2D");
    }
    const size_t rows = tensor.shape()[0];
    const size_t cols = tensor.shape()[1];
    const auto ranges = partition_evenly(rows, chunks);

    std::vector<Tensor> pieces;
    pieces.reserve(chunks);
    for (const ShardRange& range : ranges) {
        Tensor piece({range.count, cols}, 0.0);
        std::copy(tensor.data().begin() + static_cast<std::ptrdiff_t>(range.offset * cols),
                  tensor.data().begin() + static_cast<std::ptrdiff_t>(range.end() * cols),
                  piece.data().begin());
        pieces.push_back(std::move(piece));
    }
    return pieces;
}

std::vector<Tensor> DominoScheduler::split_columns(const Tensor& tensor, size_t chunks) {
    if (tensor.shape().size() != 2) {
        throw std::invalid_argument("DominoScheduler::split_columns: tensor must be 2D");
    }
    const size_t rows = tensor.shape()[0];
    const size_t cols = tensor.shape()[1];
    const auto ranges = partition_evenly(cols, chunks);

    std::vector<Tensor> pieces;
    pieces.reserve(chunks);
    for (const ShardRange& range : ranges) {
        Tensor piece({rows, range.count}, 0.0);
        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < range.count; ++c) {
                piece.data()[r * range.count + c] = tensor.data()[r * cols + range.offset + c];
            }
        }
        pieces.push_back(std::move(piece));
    }
    return pieces;
}

Tensor DominoScheduler::concat_rows(const std::vector<Tensor>& chunks) {
    if (chunks.empty()) {
        return Tensor();
    }
    const size_t cols = chunks.front().shape()[1];
    size_t rows = 0;
    for (const Tensor& chunk : chunks) {
        if (chunk.shape()[1] != cols) {
            throw std::invalid_argument("DominoScheduler::concat_rows: column mismatch");
        }
        rows += chunk.shape()[0];
    }

    Tensor result({rows, cols}, 0.0);
    size_t offset = 0;
    for (const Tensor& chunk : chunks) {
        std::copy(chunk.data().begin(), chunk.data().end(),
                  result.data().begin() + static_cast<std::ptrdiff_t>(offset));
        offset += chunk.size();
    }
    return result;
}

Tensor DominoScheduler::concat_columns(const std::vector<Tensor>& chunks) {
    if (chunks.empty()) {
        return Tensor();
    }
    const size_t rows = chunks.front().shape()[0];
    size_t cols = 0;
    for (const Tensor& chunk : chunks) {
        if (chunk.shape()[0] != rows) {
            throw std::invalid_argument("DominoScheduler::concat_columns: row mismatch");
        }
        cols += chunk.shape()[1];
    }

    Tensor result({rows, cols}, 0.0);
    size_t column_offset = 0;
    for (const Tensor& chunk : chunks) {
        const size_t chunk_cols = chunk.shape()[1];
        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < chunk_cols; ++c) {
                result.data()[r * cols + column_offset + c] = chunk.data()[r * chunk_cols + c];
            }
        }
        column_offset += chunk_cols;
    }
    return result;
}

Tensor DominoScheduler::forward_sliced(const Tensor& input,
                                       const Tensor& weight,
                                       const std::function<void(Tensor&, size_t)>& all_reduce) const {
    std::vector<Tensor> row_pieces = split_rows(input, config_.row_chunks);
    std::vector<Tensor> outputs;
    outputs.reserve(row_pieces.size());

    for (size_t chunk = 0; chunk < row_pieces.size(); ++chunk) {
        // Column slicing keeps each partial GEMM small enough that its
        // collective can be issued before the next chunk starts.
        std::vector<Tensor> weight_pieces = split_columns(weight, config_.column_chunks);
        std::vector<Tensor> partials;
        partials.reserve(weight_pieces.size());
        for (const Tensor& weight_piece : weight_pieces) {
            partials.push_back(row_pieces[chunk].matmul(weight_piece));
        }
        Tensor output = concat_columns(partials);
        if (all_reduce) {
            all_reduce(output, chunk);
        }
        outputs.push_back(std::move(output));
    }

    return concat_rows(outputs);
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
