#include "fast_persist.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <thread>

namespace ml {
namespace deep_learning {
namespace scaling {
namespace {

constexpr uint32_t kMagic = 0x46505354; // "FPST"

void write_u64(std::ostream& stream, uint64_t value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

uint64_t read_u64(std::istream& stream) {
    uint64_t value = 0;
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

} // namespace

FastPersistWriter::FastPersistWriter(Config config) : config_(std::move(config)) {
    if (config_.num_writers == 0) {
        throw std::invalid_argument("FastPersistWriter: num_writers must be > 0");
    }
    if (config_.staging_buffers == 0) {
        throw std::invalid_argument("FastPersistWriter: staging_buffers must be > 0");
    }
    std::filesystem::create_directories(config_.directory);
}

FastPersistWriter::~FastPersistWriter() {
    if (pending_.valid()) {
        pending_.wait();
    }
}

std::vector<std::vector<size_t>> FastPersistWriter::assign_shards_to_writers(size_t num_shards,
                                                                            size_t num_writers) {
    if (num_writers == 0) {
        throw std::invalid_argument("assign_shards_to_writers: num_writers must be > 0");
    }
    std::vector<std::vector<size_t>> assignment(num_writers);
    for (size_t shard = 0; shard < num_shards; ++shard) {
        assignment[shard % num_writers].push_back(shard);
    }
    return assignment;
}

std::string FastPersistWriter::file_for(const std::string& tag, size_t writer) const {
    const std::filesystem::path path =
        std::filesystem::path(config_.directory) / (tag + "." + std::to_string(writer) + ".fpst");
    return path.string();
}

PersistResult FastPersistWriter::write_impl(const std::string& tag,
                                            const std::vector<CheckpointShard>& shards) const {
    const auto started = std::chrono::steady_clock::now();
    const auto assignment = assign_shards_to_writers(shards.size(), config_.num_writers);

    PersistResult result;
    std::vector<std::thread> workers;
    std::vector<std::string> errors(config_.num_writers);

    workers.reserve(config_.num_writers);
    for (size_t writer = 0; writer < config_.num_writers; ++writer) {
        const std::string path = file_for(tag, writer);
        result.files.push_back(path);
        workers.emplace_back([&, writer, path]() {
            try {
                std::ofstream stream(path, std::ios::binary | std::ios::trunc);
                if (!stream) {
                    errors[writer] = "cannot open " + path;
                    return;
                }
                stream.write(reinterpret_cast<const char*>(&kMagic), sizeof(kMagic));
                write_u64(stream, assignment[writer].size());
                for (size_t index : assignment[writer]) {
                    const CheckpointShard& shard = shards[index];
                    write_u64(stream, shard.name.size());
                    stream.write(shard.name.data(), static_cast<std::streamsize>(shard.name.size()));
                    write_u64(stream, shard.data.size());
                    stream.write(reinterpret_cast<const char*>(shard.data.data()),
                                 static_cast<std::streamsize>(shard.data.size() * sizeof(double)));
                }
            } catch (const std::exception& error) {
                errors[writer] = error.what();
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    for (const auto& error : errors) {
        if (!error.empty()) {
            throw std::runtime_error("FastPersistWriter: " + error);
        }
    }

    for (const auto& shard : shards) {
        result.bytes += shard.bytes();
    }
    result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    return result;
}

PersistResult FastPersistWriter::write(const std::string& tag,
                                       const std::vector<CheckpointShard>& shards) {
    if (pending_.valid()) {
        wait();
    }
    return write_impl(tag, shards);
}

void FastPersistWriter::write_async(const std::string& tag, std::vector<CheckpointShard> shards) {
    if (pending_.valid()) {
        wait();
    }
    const auto policy = config_.overlap_with_compute ? std::launch::async : std::launch::deferred;
    pending_ = std::async(policy, [this, tag, moved = std::move(shards)]() {
        return write_impl(tag, moved);
    });
}

PersistResult FastPersistWriter::wait() {
    if (!pending_.valid()) {
        return {};
    }
    std::future<PersistResult> pending = std::move(pending_);
    pending_ = {};
    return pending.get();
}

std::vector<CheckpointShard> FastPersistWriter::read(const std::string& tag) const {
    std::vector<CheckpointShard> shards;
    for (size_t writer = 0; writer < config_.num_writers; ++writer) {
        std::ifstream stream(file_for(tag, writer), std::ios::binary);
        if (!stream) {
            continue;
        }
        uint32_t magic = 0;
        stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != kMagic) {
            throw std::runtime_error("FastPersistWriter::read: bad checkpoint header");
        }
        const uint64_t count = read_u64(stream);
        for (uint64_t i = 0; i < count; ++i) {
            CheckpointShard shard;
            const uint64_t name_length = read_u64(stream);
            shard.name.resize(static_cast<size_t>(name_length));
            stream.read(shard.name.data(), static_cast<std::streamsize>(name_length));
            const uint64_t values = read_u64(stream);
            shard.data.resize(static_cast<size_t>(values));
            stream.read(reinterpret_cast<char*>(shard.data.data()),
                        static_cast<std::streamsize>(values * sizeof(double)));
            shards.push_back(std::move(shard));
        }
    }
    std::sort(shards.begin(), shards.end(), [](const CheckpointShard& a, const CheckpointShard& b) {
        return a.name < b.name;
    });
    return shards;
}

double FastPersistWriter::estimated_seconds(size_t bytes,
                                            double per_device_bytes_per_second,
                                            size_t num_writers,
                                            double compute_seconds,
                                            bool overlap_with_compute) {
    if (per_device_bytes_per_second <= 0.0 || num_writers == 0) {
        return 0.0;
    }
    const double aggregate = per_device_bytes_per_second * static_cast<double>(num_writers);
    const double write_seconds = static_cast<double>(bytes) / aggregate;
    return overlap_with_compute ? std::max(compute_seconds, write_seconds)
                                : compute_seconds + write_seconds;
}

} // namespace scaling
} // namespace deep_learning
} // namespace ml
