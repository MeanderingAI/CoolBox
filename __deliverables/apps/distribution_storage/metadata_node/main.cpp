#include "metadata_server.h"
#include "dfs_common.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// metadata_node: name-node that tracks the directory tree and block locations.
// Accepts node registrations via --nodes n1,n2,... and serves namespace
// operations interactively from stdin.

int main(int argc, char* argv[]) {
    std::vector<std::string> nodes;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--nodes" && i + 1 < argc) {
            std::string tok;
            std::istringstream ss(argv[++i]);
            while (std::getline(ss, tok, ','))
                if (!tok.empty()) nodes.push_back(tok);
        }
    }

    trekker::dfs::MetadataServer ms;
    ms.mkdir("/");
    for (const auto& n : nodes)
        ms.register_node(n, 1024ULL * 1024 * 1024);

    std::cout << "[metadata_node] registered " << nodes.size() << " storage node(s)\n";
    std::cout << "[metadata_node] ready\n";

    std::string line;
    while (std::cout << "meta> " && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        if (line == "quit" || line == "exit") break;

        std::istringstream ss(line);
        std::string cmd; ss >> cmd;

        if (cmd == "ls") {
            std::string path = "/"; ss >> path;
            auto entries = ms.listdir(path);
            if (!entries) { std::cout << "not a directory or not found: " << path << "\n"; continue; }
            dfs_app::print_listing(*entries);

        } else if (cmd == "stat") {
            std::string path; ss >> path;
            auto s = ms.stat(path);
            if (!s) { std::cout << "not found: " << path << "\n"; continue; }
            dfs_app::print_stat(*s);

        } else if (cmd == "mkdir") {
            std::string path; ss >> path;
            std::cout << (ms.mkdir(path) ? "ok" : "failed") << "\n";

        } else if (cmd == "rm") {
            std::string path; ss >> path;
            std::cout << (ms.remove(path) ? "ok" : "failed (not empty or not found)") << "\n";

        } else if (cmd == "nodes") {
            for (const auto& n : ms.live_nodes()) std::cout << n << "\n";

        } else if (cmd == "pick") {
            std::cout << ms.pick_node_for_write() << "\n";

        } else {
            std::cout << "commands: ls [path]  stat <path>  mkdir <path>  rm <path>  nodes  pick  exit\n";
        }
    }
    return 0;
}
