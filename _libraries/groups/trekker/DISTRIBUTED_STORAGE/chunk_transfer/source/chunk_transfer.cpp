#include "chunk_transfer.h"
#include <cstring>

namespace trekker {
namespace dfs {

// ── ChunkSender ───────────────────────────────────────────────────────────────

ChunkSender::ChunkSender(WriteFn write_fn) : write_fn_(std::move(write_fn)) {}

TransferResult ChunkSender::send(const BlockId& id,
                                 const std::vector<std::uint8_t>& data) {
    // Frame: [4 magic][32 block_id][8 length][payload]
    const std::uint64_t len = static_cast<std::uint64_t>(data.size());
    std::vector<std::uint8_t> frame;
    frame.reserve(4 + 32 + 8 + data.size());

    const std::uint32_t magic_be = kMagic;
    const auto* mb = reinterpret_cast<const std::uint8_t*>(&magic_be);
    frame.insert(frame.end(), mb, mb + 4);
    frame.insert(frame.end(), id.begin(), id.end());
    const auto* lb = reinterpret_cast<const std::uint8_t*>(&len);
    frame.insert(frame.end(), lb, lb + 8);
    frame.insert(frame.end(), data.begin(), data.end());

    if (!write_fn_(frame.data(), frame.size()))
        return {false, 0, "write_fn returned false"};
    return {true, data.size(), ""};
}

// ── ChunkReceiver ─────────────────────────────────────────────────────────────

ChunkReceiver::ChunkReceiver(ReadFn read_fn) : read_fn_(std::move(read_fn)) {}

std::optional<BlockId> ChunkReceiver::receive(BlockStore& store) {
    auto read_exact = [&](std::uint8_t* buf, std::size_t n) -> bool {
        std::size_t done = 0;
        while (done < n) {
            const std::size_t r = read_fn_(buf + done, n - done);
            if (r == 0) return false;
            done += r;
        }
        return true;
    };

    std::uint32_t magic = 0;
    if (!read_exact(reinterpret_cast<std::uint8_t*>(&magic), 4)) return std::nullopt;
    if (magic != ChunkSender::kMagic) return std::nullopt;

    BlockId id{};
    if (!read_exact(id.data(), 32)) return std::nullopt;

    std::uint64_t len = 0;
    if (!read_exact(reinterpret_cast<std::uint8_t*>(&len), 8)) return std::nullopt;

    std::vector<std::uint8_t> payload(static_cast<std::size_t>(len));
    if (len > 0 && !read_exact(payload.data(), payload.size())) return std::nullopt;

    store.put(payload.data(), payload.size());
    return id;
}

// ── ChunkPipeline ─────────────────────────────────────────────────────────────

ChunkPipeline::ChunkPipeline(BlockStore* src, BlockStore* dst)
    : src_(src), dst_(dst) {}

int ChunkPipeline::transfer(const std::vector<BlockId>& ids) {
    int ok = 0;
    for (const auto& id : ids) {
        if (transfer_one(id).ok) ++ok;
    }
    return ok;
}

TransferResult ChunkPipeline::transfer_one(const BlockId& id) {
    auto data = src_->get(id);
    if (!data) return {false, 0, "block not found in source"};
    dst_->put(data->data(), data->size());
    return {true, data->size(), ""};
}

} // namespace dfs
} // namespace trekker
