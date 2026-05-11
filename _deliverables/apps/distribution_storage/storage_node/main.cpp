#include "block_store.h"
#include "metadata_server.h"
#include "dfs_common.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

// storage_node: a data-node that holds blocks and registers with the
// MetadataServer.  In local/in-process mode both live in the same process for
// demonstration.

int main(int argc, char* argv[]) {
    std::string node_id   = "storage-node-0";
    std::size_t capacity  = 512ULL * 1024 * 1024; // 512 MB default

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--node-id" && i + 1 < argc)   node_id  = argv[++i];
        if (arg == "--capacity" && i + 1 < argc) {
            const long long c = std::atoll(argv[++i]);
            if (c > 0) capacity = static_cast<std::size_t>(c);
        }
    }

    trekker::dfs::BlockStore::Config cfg;
    cfg.max_blocks   = capacity / cfg.block_size;
    trekker::dfs::BlockStore store(cfg);

    trekker::dfs::MetadataServer meta;
    meta.register_node(node_id, capacity);
    meta.mkdir("/");

    std::cout << "[storage_node] node_id=" << node_id
              << "  capacity=" << dfs_app::format_size(capacity)
              << "\n";
    std::cout << "[storage_node] ready (in-process mode — use dfs_shell to interact)\n";

    // Interactive command loop: accepts "put <hex_id> <data>", "get <hex_id>", "quit"
    std::string line;
    while (std::cout << "storage> " && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;

        if (line.rfind("put ", 0) == 0) {
            const std::string data = line.substr(4);
            const trekker::dfs::BlockId id =
                store.put(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
            std::cout << "stored: " << trekker::dfs::block_id_to_hex(id) << "\n";
        } else if (line.rfind("get ", 0) == 0) {
            const trekker::dfs::BlockId id =
                trekker::dfs::block_id_from_hex(line.substr(4));
            auto blk = store.get(id);
            if (blk)
                std::cout << "data: " << std::string(reinterpret_cast<const char*>(blk->data()),
                                                     blk->size()) << "\n";
            else
                std::cout << "block not found\n";
        } else if (line == "stats") {
            std::cout << "blocks=" << store.block_count()
                      << "  used=" << dfs_app::format_size(store.total_bytes()) << "\n";
        } else {
            std::cout << "commands: put <data>  get <hex_id>  stats  exit\n";
        }
    }
    return 0;
}
