#include "chunk_transfer.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

using namespace trekker::dfs;

static int passed = 0, failed = 0;
#define EXPECT_TRUE(expr)  do { if (expr) { ++passed; } else { ++failed; std::cerr << "FAIL: " #expr " at line " << __LINE__ << "\n"; } } while(0)
#define EXPECT_EQ(a, b)    EXPECT_TRUE((a) == (b))

int main() {
    // ChunkPipeline: transfer_one copies a block between in-process stores
    {
        BlockStore src, dst;
        const std::string data = "chunk pipeline transfer payload";
        const BlockId id = src.put(reinterpret_cast<const std::uint8_t*>(data.data()),
                                   data.size());

        ChunkPipeline pipe(&src, &dst);
        TransferResult r = pipe.transfer_one(id);
        EXPECT_TRUE(r.ok);
        EXPECT_EQ(r.bytes_transferred, data.size());

        auto got = dst.get(id);
        EXPECT_TRUE(got.has_value());
        EXPECT_EQ(std::string(reinterpret_cast<const char*>(got->data()), got->size()), data);
    }

    // ChunkPipeline: transfer() multiple blocks
    {
        BlockStore src, dst;
        const std::string d1 = "alpha", d2 = "beta", d3 = "gamma";
        BlockId i1 = src.put(reinterpret_cast<const std::uint8_t*>(d1.data()), d1.size());
        BlockId i2 = src.put(reinterpret_cast<const std::uint8_t*>(d2.data()), d2.size());
        BlockId i3 = src.put(reinterpret_cast<const std::uint8_t*>(d3.data()), d3.size());

        ChunkPipeline pipe(&src, &dst);
        int n = pipe.transfer({i1, i2, i3});
        EXPECT_EQ(n, 3);
        EXPECT_EQ(dst.block_count(), 3u);
    }

    // ChunkPipeline: missing block returns error
    {
        BlockStore src, dst;
        BlockId missing_id{};
        missing_id[0] = 0xFF;

        ChunkPipeline pipe(&src, &dst);
        TransferResult r = pipe.transfer_one(missing_id);
        EXPECT_TRUE(!r.ok);
        EXPECT_TRUE(!r.error.empty());
    }

    // ChunkSender / ChunkReceiver: frame-based in-memory roundtrip
    {
        BlockStore src, dst;
        const std::string data = "frame roundtrip payload";
        const BlockId id = src.put(reinterpret_cast<const std::uint8_t*>(data.data()),
                                   data.size());
        const auto block_data = src.get(id);
        EXPECT_TRUE(block_data.has_value());

        // Shared buffer simulating a byte stream.
        std::vector<std::uint8_t> wire;

        ChunkSender sender([&](const std::uint8_t* buf, std::size_t len) -> bool {
            wire.insert(wire.end(), buf, buf + len);
            return true;
        });
        auto sr = sender.send(id, *block_data);
        EXPECT_TRUE(sr.ok);

        std::size_t read_pos = 0;
        ChunkReceiver receiver([&](std::uint8_t* buf, std::size_t len) -> std::size_t {
            const std::size_t avail = wire.size() - read_pos;
            const std::size_t n = std::min(len, avail);
            std::memcpy(buf, wire.data() + read_pos, n);
            read_pos += n;
            return n;
        });
        auto received_id = receiver.receive(dst);
        EXPECT_TRUE(received_id.has_value());
        EXPECT_EQ(*received_id, id);
        auto got = dst.get(id);
        EXPECT_TRUE(got.has_value());
        EXPECT_EQ(std::string(reinterpret_cast<const char*>(got->data()), got->size()), data);
    }

    std::cout << "chunk_transfer tests: " << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
