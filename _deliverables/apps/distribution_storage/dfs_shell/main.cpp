#include "dfs_client.h"
#include "block_store.h"
#include "metadata_server.h"
#include "dfs_common.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

// dfs_shell: interactive REPL that drives DfsClient.
// All storage is in-process (single-host demo mode).
// Commands: ls, stat, mkdir, put, get, cat, rm, exit

static void print_help() {
    std::cout <<
        "  ls <path>               list directory\n"
        "  stat <path>             file/directory info\n"
        "  mkdir <path>            create directory (with parents)\n"
        "  put <local> <dfs>       upload local file to DFS path\n"
        "  get <dfs> <local>       download DFS path to local file\n"
        "  cat <dfs>               print DFS file contents\n"
        "  rm <path>               remove file or empty directory\n"
        "  help                    show this message\n"
        "  exit                    quit\n";
}

int main(int argc, char* /*argv*/[]) {
    // In-process metadata + block store.
    trekker::dfs::MetadataServer ms;
    trekker::dfs::BlockStore     bs;
    ms.register_node("local", 4ULL * 1024 * 1024 * 1024);
    ms.mkdir("/");

    trekker::dfs::DfsClient client(&ms, &bs);

    std::cout << "dfs_shell — type 'help' for commands\n";

    std::string line;
    while (std::cout << "dfs> " && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string cmd; ss >> cmd;

        if (cmd == "exit" || cmd == "quit") break;

        if (cmd == "help") {
            print_help();

        } else if (cmd == "ls") {
            std::string path = "/"; ss >> path;
            auto entries = client.listdir(path);
            if (!entries) { std::cout << "not found or not a directory: " << path << "\n"; continue; }
            if (entries->empty()) std::cout << "(empty)\n";
            else dfs_app::print_listing(*entries);

        } else if (cmd == "stat") {
            std::string path; ss >> path;
            auto s = client.stat(path);
            if (!s) { std::cout << "not found: " << path << "\n"; continue; }
            dfs_app::print_stat(*s);

        } else if (cmd == "mkdir") {
            std::string path; ss >> path;
            if (path.empty()) { std::cout << "usage: mkdir <path>\n"; continue; }
            const auto err = client.mkdir(path, /*parents=*/true);
            std::cout << (err == trekker::dfs::DfsError::OK ? "ok" : "error") << "\n";

        } else if (cmd == "put") {
            std::string local_path, dfs_path;
            ss >> local_path >> dfs_path;
            if (local_path.empty() || dfs_path.empty()) {
                std::cout << "usage: put <local> <dfs>\n"; continue;
            }
            std::ifstream f(local_path, std::ios::binary);
            if (!f) { std::cout << "cannot open local file: " << local_path << "\n"; continue; }
            const std::vector<std::uint8_t> data(
                (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            if (!ms.exists(dfs_path)) ms.create(dfs_path);
            const auto err = client.write_all(dfs_path, data.data(), data.size());
            if (err == trekker::dfs::DfsError::OK)
                std::cout << "uploaded " << dfs_app::format_size(data.size()) << "\n";
            else
                std::cout << "write error\n";

        } else if (cmd == "get") {
            std::string dfs_path, local_path;
            ss >> dfs_path >> local_path;
            if (dfs_path.empty() || local_path.empty()) {
                std::cout << "usage: get <dfs> <local>\n"; continue;
            }
            std::vector<std::uint8_t> data;
            const auto err = client.read_all(dfs_path, data);
            if (err != trekker::dfs::DfsError::OK) { std::cout << "read error\n"; continue; }
            std::ofstream f(local_path, std::ios::binary);
            if (!f) { std::cout << "cannot write local file: " << local_path << "\n"; continue; }
            f.write(reinterpret_cast<const char*>(data.data()), data.size());
            std::cout << "downloaded " << dfs_app::format_size(data.size()) << "\n";

        } else if (cmd == "cat") {
            std::string dfs_path; ss >> dfs_path;
            std::vector<std::uint8_t> data;
            const auto err = client.read_all(dfs_path, data);
            if (err != trekker::dfs::DfsError::OK) { std::cout << "read error\n"; continue; }
            std::cout.write(reinterpret_cast<const char*>(data.data()), data.size());
            if (!data.empty() && data.back() != '\n') std::cout << "\n";

        } else if (cmd == "rm") {
            std::string path; ss >> path;
            const auto err = client.rm(path);
            std::cout << (err == trekker::dfs::DfsError::OK ? "ok" : "not found or error") << "\n";

        } else {
            std::cout << "unknown command: " << cmd << " — type 'help'\n";
        }
    }
    return 0;
}
