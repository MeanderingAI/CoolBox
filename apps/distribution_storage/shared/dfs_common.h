#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include "metadata_server.h"

namespace dfs_app {

// Parse "host:port" into host and port (default port if missing)
inline bool parse_host_port(const std::string& addr,
                             std::string& host, std::uint16_t& port,
                             std::uint16_t default_port = 7100) {
    const auto pos = addr.rfind(':');
    if (pos == std::string::npos) {
        host = addr;
        port = default_port;
        return !host.empty();
    }
    host = addr.substr(0, pos);
    try {
        const int p = std::stoi(addr.substr(pos + 1));
        if (p < 1 || p > 65535) return false;
        port = static_cast<std::uint16_t>(p);
    } catch (...) { return false; }
    return !host.empty();
}

// Human-readable byte size
inline std::string format_size(std::size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double v = static_cast<double>(bytes);
    int idx = 0;
    while (v >= 1024.0 && idx < 4) { v /= 1024.0; ++idx; }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f %s", v, units[idx]);
    return buf;
}

// Print a single Stat entry
inline void print_stat(const trekker::dfs::Stat& s) {
    std::cout << (s.type == trekker::dfs::NodeType::Directory ? "d" : "-")
              << "  " << s.path
              << "  size=" << format_size(s.size)
              << "  replication=" << s.replication
              << "\n";
}

// Print a directory listing
inline void print_listing(const std::vector<trekker::dfs::DirEntry>& entries) {
    for (const auto& e : entries) {
        std::cout << (e.type == trekker::dfs::NodeType::Directory ? "[d] " : "    ")
                  << e.name << "\n";
    }
}

} // namespace dfs_app
