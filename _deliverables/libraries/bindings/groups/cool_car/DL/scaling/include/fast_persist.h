#ifndef ML_DEEP_LEARNING_SCALING_FAST_PERSIST_H
#define ML_DEEP_LEARNING_SCALING_FAST_PERSIST_H

#include <cstddef>
#include <future>
#include <string>
#include <vector>

namespace ml {
namespace deep_learning {
namespace scaling {

/// @brief Parallel, overlapped checkpoint persistence.
///
/// Implements the write parallelism, double buffering and compute overlap of
/// Wang et al., "FastPersist: Accelerating Model Checkpointing in Deep
/// Learning" (arXiv:2406.13768), citation key @c wang_fastpersist_2024.

/// One named piece of checkpoint state, e.g. a parameter or optimizer shard.
struct CheckpointShard {
    std::string name;
    std::vector<double> data;

    size_t bytes() const { return data.size() * sizeof(double); }
};

struct PersistResult {
    std::vector<std::string> files;
    size_t bytes = 0;
    double seconds = 0.0;
    /// Effective aggregate throughput in bytes per second.
    double throughput() const { return seconds > 0.0 ? static_cast<double>(bytes) / seconds : 0.0; }
};

class FastPersistWriter {
public:
    struct Config {
        std::string directory = ".";
        /// One writer per available SSD; checkpoint data is striped across them.
        size_t num_writers = 4;
        /// Double buffering: staging buffers reused while a write is in flight.
        size_t staging_buffers = 2;
        /// Issues the write on a background thread so it overlaps the next
        /// forward/backward pass.
        bool overlap_with_compute = true;
    };

    explicit FastPersistWriter(Config config);
    ~FastPersistWriter();

    FastPersistWriter(const FastPersistWriter&) = delete;
    FastPersistWriter& operator=(const FastPersistWriter&) = delete;

    /// Writes @p shards synchronously, striped across the configured writers.
    PersistResult write(const std::string& tag, const std::vector<CheckpointShard>& shards);

    /// Starts a write in the background and returns immediately, so the caller
    /// can continue training while the previous checkpoint drains to storage.
    void write_async(const std::string& tag, std::vector<CheckpointShard> shards);

    /// Blocks until the outstanding asynchronous write completes.
    PersistResult wait();
    bool has_pending_write() const { return pending_.valid(); }

    std::vector<CheckpointShard> read(const std::string& tag) const;

    /// Round-robin striping of shards over writers.
    static std::vector<std::vector<size_t>> assign_shards_to_writers(size_t num_shards,
                                                                     size_t num_writers);

    /// Serial write time versus the overlapped, parallel time FastPersist achieves.
    static double estimated_seconds(size_t bytes,
                                    double per_device_bytes_per_second,
                                    size_t num_writers,
                                    double compute_seconds,
                                    bool overlap_with_compute);

private:
    PersistResult write_impl(const std::string& tag, const std::vector<CheckpointShard>& shards) const;
    std::string file_for(const std::string& tag, size_t writer) const;

    Config config_;
    std::future<PersistResult> pending_;
};

} // namespace scaling
} // namespace deep_learning
} // namespace ml

#endif // ML_DEEP_LEARNING_SCALING_FAST_PERSIST_H
