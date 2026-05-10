#include "replication.h"
#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace trekker::dfs;

static int passed = 0, failed = 0;
#define EXPECT_TRUE(expr)  do { if (expr) { ++passed; } else { ++failed; std::cerr << "FAIL: " #expr " at line " << __LINE__ << "\n"; } } while(0)
#define EXPECT_EQ(a, b)    EXPECT_TRUE((a) == (b))

int main() {
    // ReplicationPlanner — RoundRobin distributes across nodes
    {
        ReplicationConfig cfg;
        cfg.replication_factor = 3;
        cfg.strategy = PlacementStrategy::RoundRobin;
        ReplicationPlanner planner(cfg);

        const std::vector<std::string> live = {"n1", "n2", "n3", "n4"};
        const std::vector<std::string> existing = {};
        const auto targets = planner.plan(live, existing, 3);
        EXPECT_EQ(targets.size(), 3u);
        // All selected from live, none duplicated.
        std::set<std::string> uniq(targets.begin(), targets.end());
        EXPECT_EQ(uniq.size(), 3u);
    }

    // BlockReplicator — replicate copies block to targets
    {
        std::map<std::string, BlockStore> stores;
        stores["src"];
        stores["dst1"];
        stores["dst2"];

        auto provider = [&](const std::string& id) -> BlockStore* {
            auto it = stores.find(id);
            return it == stores.end() ? nullptr : &it->second;
        };

        const std::string data = "replication test payload";
        const BlockId id = stores["src"].put(
            reinterpret_cast<const std::uint8_t*>(data.data()), data.size());

        BlockReplicator rep(provider);
        const int n = rep.replicate(id, "src", {"dst1", "dst2"});
        EXPECT_EQ(n, 2);
        EXPECT_TRUE(stores["dst1"].get(id).has_value());
        EXPECT_TRUE(stores["dst2"].get(id).has_value());
    }

    // BlockReplicator — verify detects missing block
    {
        std::map<std::string, BlockStore> stores;
        stores["n1"];
        stores["n2"];

        auto provider = [&](const std::string& id) -> BlockStore* {
            auto it = stores.find(id);
            return it == stores.end() ? nullptr : &it->second;
        };

        const std::string data = "verify test";
        const BlockId id = stores["n1"].put(
            reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
        // n2 does NOT have the block.

        BlockReplicator rep(provider);
        const auto missing = rep.verify(id, {"n1", "n2"});
        EXPECT_EQ(missing.size(), 1u);
        EXPECT_EQ(missing[0], std::string("n2"));
    }

    std::cout << "replication tests: " << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
