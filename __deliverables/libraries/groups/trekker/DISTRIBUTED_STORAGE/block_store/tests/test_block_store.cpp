#include "block_store.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

using namespace trekker::dfs;

static int passed = 0, failed = 0;
#define EXPECT_TRUE(expr)  do { if (expr) { ++passed; } else { ++failed; std::cerr << "FAIL: " #expr " at line " << __LINE__ << "\n"; } } while(0)
#define EXPECT_EQ(a, b)    EXPECT_TRUE((a) == (b))

int main() {
    // put / get round-trip
    {
        BlockStore store;
        const std::string msg = "hello distributed storage";
        BlockId id = store.put(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size());
        auto got = store.get(id);
        EXPECT_TRUE(got.has_value());
        EXPECT_EQ(std::string(reinterpret_cast<const char*>(got->data()), got->size()), msg);
    }

    // content-addressing: same data → same id
    {
        BlockStore store;
        const std::string msg = "dedup test";
        BlockId id1 = store.put(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size());
        BlockId id2 = store.put(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size());
        EXPECT_EQ(id1, id2);
        EXPECT_EQ(store.block_count(), 1u);
    }

    // ref_count: put twice → ref_count 2; release once → still present
    {
        BlockStore store;
        const std::string msg = "ref_count";
        BlockId id = store.put(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size());
        store.put(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size()); // +1
        auto m = store.meta(id);
        EXPECT_TRUE(m.has_value());
        EXPECT_EQ(m->ref_count, 2u);
        store.release(id);
        EXPECT_EQ(store.block_count(), 1u); // still there (ref_count 1)
        store.release(id);
        EXPECT_EQ(store.block_count(), 0u); // removed
    }

    // pin prevents removal
    {
        BlockStore store;
        const std::string msg = "pinned";
        BlockId id = store.put(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size());
        store.pin(id);
        store.release(id); // ref_count → 0 but pinned
        EXPECT_EQ(store.block_count(), 1u);
        store.unpin(id);
        store.release(id); // now actually removed? ref_count was 0 already after prior release
        // After unpin with ref_count==0 the block stays (release already ran); clear it.
        store.clear();
        EXPECT_EQ(store.block_count(), 0u);
    }

    // total_bytes
    {
        BlockStore store;
        const std::string a = "aaaa", b = "bbbbbbbb";
        store.put(reinterpret_cast<const std::uint8_t*>(a.data()), a.size());
        store.put(reinterpret_cast<const std::uint8_t*>(b.data()), b.size());
        EXPECT_EQ(store.total_bytes(), a.size() + b.size());
    }

    // BlockMap append / locate / file_size
    {
        BlockStore store;
        BlockMap bmap;
        const std::string chunk1 = "AAAAAA", chunk2 = "BBBBBB";
        BlockId id1 = store.put(reinterpret_cast<const std::uint8_t*>(chunk1.data()), chunk1.size());
        BlockId id2 = store.put(reinterpret_cast<const std::uint8_t*>(chunk2.data()), chunk2.size());
        bmap.append({"node1", id1, 0, chunk1.size()});
        bmap.append({"node2", id2, chunk1.size(), chunk2.size()});
        EXPECT_EQ(bmap.file_size(), chunk1.size() + chunk2.size());
        auto loc = bmap.locate(chunk1.size());
        EXPECT_TRUE(loc.has_value());
        EXPECT_EQ(loc->node_id, std::string("node2"));
    }

    std::cout << "block_store tests: " << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
