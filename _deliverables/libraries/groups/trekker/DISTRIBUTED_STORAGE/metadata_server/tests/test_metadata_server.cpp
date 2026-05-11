#include "metadata_server.h"
#include <iostream>

using namespace trekker::dfs;

static int passed = 0, failed = 0;
#define EXPECT_TRUE(expr)  do { if (expr) { ++passed; } else { ++failed; std::cerr << "FAIL: " #expr " at line " << __LINE__ << "\n"; } } while(0)
#define EXPECT_EQ(a, b)    EXPECT_TRUE((a) == (b))

int main() {
    // mkdir / listdir
    {
        MetadataServer ms;
        EXPECT_TRUE(ms.mkdir("/data"));
        EXPECT_TRUE(ms.exists("/data"));
        auto entries = ms.listdir("/");
        EXPECT_TRUE(entries.has_value());
        EXPECT_EQ(entries->size(), 1u);
        EXPECT_EQ((*entries)[0].name, std::string("data"));
    }

    // create / stat
    {
        MetadataServer ms;
        ms.mkdir("/files");
        EXPECT_TRUE(ms.create("/files/hello.txt"));
        auto s = ms.stat("/files/hello.txt");
        EXPECT_TRUE(s.has_value());
        EXPECT_EQ(s->type, NodeType::File);
        EXPECT_EQ(s->size, 0u);
    }

    // rename
    {
        MetadataServer ms;
        ms.mkdir("/a");
        ms.create("/a/orig.txt");
        EXPECT_TRUE(ms.rename("/a/orig.txt", "/a/renamed.txt"));
        EXPECT_TRUE(!ms.exists("/a/orig.txt"));
        EXPECT_TRUE(ms.exists("/a/renamed.txt"));
    }

    // remove
    {
        MetadataServer ms;
        ms.mkdir("/del");
        ms.create("/del/gone.txt");
        EXPECT_TRUE(ms.remove("/del/gone.txt"));
        EXPECT_TRUE(!ms.exists("/del/gone.txt"));
    }

    // append_block / block_locations
    {
        MetadataServer ms;
        ms.mkdir("/store");
        ms.create("/store/data.bin");
        BlockId bid{};
        bid[0] = 0xAB;
        BlockLocation loc{"node1", bid, 0, 1024};
        EXPECT_TRUE(ms.append_block("/store/data.bin", loc));
        auto locs = ms.block_locations("/store/data.bin");
        EXPECT_TRUE(locs.has_value());
        EXPECT_EQ(locs->size(), 1u);
        EXPECT_EQ((*locs)[0].node_id, std::string("node1"));
        // stat should reflect size
        auto s = ms.stat("/store/data.bin");
        EXPECT_EQ(s->size, 1024u);
    }

    // register_node / pick_node_for_write
    {
        MetadataServer ms;
        ms.register_node("nodeA", 1000);
        ms.register_node("nodeB", 2000);
        const std::string n1 = ms.pick_node_for_write();
        const std::string n2 = ms.pick_node_for_write();
        EXPECT_TRUE(!n1.empty());
        EXPECT_TRUE(!n2.empty());
        const auto live = ms.live_nodes();
        EXPECT_EQ(live.size(), 2u);
    }

    std::cout << "metadata_server tests: " << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
