#ifndef TREKKER_DFS_CHUNK_TRANSFER_H
#define TREKKER_DFS_CHUNK_TRANSFER_H

#include "block_store.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace trekker {
namespace dfs {

// ── Transfer result ───────────────────────────────────────────────────────────

struct TransferResult {
    bool        ok;
    std::size_t bytes_transferred;
    std::string error;
};

// ── ChunkSender ───────────────────────────────────────────────────────────────
// Abstracts serialising a block for sending over a byte stream.
// In production, this wraps a TCP socket write.
// Here it operates on an in-memory byte vector for portability.

class ChunkSender {
public:
    // Callback that receives the framed packet bytes (header + payload).
    using WriteFn = std::function<bool(const std::uint8_t* buf, std::size_t len)>;

    explicit ChunkSender(WriteFn write_fn);

    // Frame and send block data.
    // Frame format: [4-byte magic][32-byte block_id][8-byte length][payload]
    TransferResult send(const BlockId& id,
                        const std::vector<std::uint8_t>& data);

    static constexpr std::uint32_t kMagic = 0xDF510000U; // "DFS BLOCK" sentinel

private:
    WriteFn write_fn_;
};

// ── ChunkReceiver ─────────────────────────────────────────────────────────────
// Abstracts deserialising a block from a byte stream and writing into a
// BlockStore.

class ChunkReceiver {
public:
    // Callback that reads up to `len` bytes into `buf`.
    using ReadFn = std::function<std::size_t(std::uint8_t* buf, std::size_t len)>;

    explicit ChunkReceiver(ReadFn read_fn);

    // Receive one framed block and store it.
    // Returns the BlockId on success; nullopt on error.
    std::optional<BlockId> receive(BlockStore& store);

private:
    ReadFn read_fn_;
};

// ── ChunkPipeline ─────────────────────────────────────────────────────────────
// Transfers all blocks of a file from a source BlockStore to a sink BlockStore
// (in-process).  Intended for test and single-host usage; replace with real
// ChunkSender/Receiver pair for multi-host deployment.

class ChunkPipeline {
public:
    ChunkPipeline(BlockStore* src, BlockStore* dst);

    // Transfer a set of block ids.
    // Returns number of blocks successfully copied.
    int transfer(const std::vector<BlockId>& ids);

    // Transfer a single block.
    TransferResult transfer_one(const BlockId& id);

private:
    BlockStore* src_;
    BlockStore* dst_;
};

} // namespace dfs
} // namespace trekker

#endif // TREKKER_DFS_CHUNK_TRANSFER_H
