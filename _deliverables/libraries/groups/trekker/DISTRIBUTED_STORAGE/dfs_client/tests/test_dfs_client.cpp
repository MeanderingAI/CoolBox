#include "dfs_client.h"
#include "block_store.h"
#include "metadata_server.h"
#include <cstring>
#include <iostream>
#include <string>

using namespace trekker::dfs;

static int passed = 0, failed = 0;
#define EXPECT_TRUE(expr)  do { if (expr) { ++passed; } else { ++failed; std::cerr << "FAIL: " #expr " at line " << __LINE__ << "\n"; } } while(0)
#define EXPECT_EQ(a, b)    EXPECT_TRUE((a) == (b))

// Helpers
static MetadataServer ms;
static BlockStore     bs;

int main() {
    DfsClient client(&ms, &bs);
    ms.register_node("local", 1024ULL * 1024 * 1024);

    // write_all / read_all round-trip
    {
        ms.mkdir("/test");
        ms.create("/test/file.txt");
        const std::string data = "Hello, distributed filesystem!";
        auto err = client.write_all("/test/file.txt",
                       reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
        EXPECT_EQ(err, DfsError::OK);

        std::vector<std::uint8_t> out;
        err = client.read_all("/test/file.txt", out);
        EXPECT_EQ(err, DfsError::OK);
        const std::string got(reinterpret_cast<const char*>(out.data()), out.size());
        EXPECT_EQ(got, data);
    }

    // open + write + seek + read
    {
        ms.create("/test/seek.bin");
        auto fh = client.open("/test/seek.bin", O_DFS_WRONLY);
        EXPECT_TRUE(fh != nullptr);
        const std::string part1 = "AAAA", part2 = "BBBB";
        fh->write(reinterpret_cast<const std::uint8_t*>(part1.data()), part1.size());
        fh->write(reinterpret_cast<const std::uint8_t*>(part2.data()), part2.size());
        fh->close();

        auto rfh = client.open("/test/seek.bin", O_DFS_RDONLY);
        EXPECT_TRUE(rfh != nullptr);
        rfh->seek(4, SeekMode::Begin); // skip to part2
        std::uint8_t buf[4];
        const std::int64_t n = rfh->read(buf, 4);
        EXPECT_EQ(n, 4);
        EXPECT_EQ(std::string(reinterpret_cast<const char*>(buf), 4), part2);
        rfh->close();
    }

    // mkdir parents
    {
        auto err = client.mkdir("/a/b/c", true);
        EXPECT_EQ(err, DfsError::OK);
        EXPECT_TRUE(client.exists("/a/b/c"));
    }

    // stat
    {
        auto s = client.stat("/test/file.txt");
        EXPECT_TRUE(s.has_value());
        EXPECT_EQ(s->type, NodeType::File);
    }

    // listdir
    {
        auto entries = client.listdir("/test");
        EXPECT_TRUE(entries.has_value());
        EXPECT_TRUE(entries->size() >= 2u);
    }

    // rm
    {
        ms.create("/test/todelete.txt");
        EXPECT_EQ(client.rm("/test/todelete.txt"), DfsError::OK);
        EXPECT_TRUE(!client.exists("/test/todelete.txt"));
    }

    std::cout << "dfs_client tests: " << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
