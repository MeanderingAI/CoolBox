#include "metadata_server.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace trekker {
namespace dfs {

static std::int64_t now_epoch() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// ── DirTree ───────────────────────────────────────────────────────────────────

DirTree::DirTree() {
    INode root;
    root.inode = next_inode_++;
    root.type  = NodeType::Directory;
    root.owner = "root";
    root.mode  = 0755;
    root.mtime = now_epoch();
    root.atime = root.mtime;
    nodes_["/"] = root;
}

std::string DirTree::normalise(const std::string& p) const {
    if (p.empty()) return "/";
    std::string s = p;
    // Ensure absolute
    if (s[0] != '/') s = "/" + s;
    // Remove trailing slash except for root
    if (s.size() > 1 && s.back() == '/') s.pop_back();
    // Collapse double slashes
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '/' && !out.empty() && out.back() == '/') continue;
        out += c;
    }
    return out;
}

std::pair<std::string, std::string> DirTree::split_path(const std::string& p) const {
    const std::string np = normalise(p);
    const auto pos = np.rfind('/');
    if (pos == 0) return {"/", np.substr(1)};
    return {np.substr(0, pos), np.substr(pos + 1)};
}

bool DirTree::mkdir(const std::string& path, const std::string& owner, std::uint16_t mode) {
    const std::string np = normalise(path);
    if (nodes_.count(np)) return false;
    auto [par, name] = split_path(np);
    auto pit = nodes_.find(par);
    if (pit == nodes_.end() || pit->second.type != NodeType::Directory) return false;

    INode node;
    node.inode = next_inode_++;
    node.type  = NodeType::Directory;
    node.owner = owner;
    node.mode  = mode;
    node.mtime = now_epoch();
    node.atime = node.mtime;
    pit->second.children[name] = node.inode;
    nodes_[np] = node;
    return true;
}

bool DirTree::mkfile(const std::string& path, const std::string& owner,
                     std::uint16_t mode, std::uint16_t replication) {
    const std::string np = normalise(path);
    if (nodes_.count(np)) return false;
    auto [par, name] = split_path(np);
    auto pit = nodes_.find(par);
    if (pit == nodes_.end() || pit->second.type != NodeType::Directory) return false;

    INode node;
    node.inode       = next_inode_++;
    node.type        = NodeType::File;
    node.owner       = owner;
    node.mode        = mode;
    node.replication = replication;
    node.mtime       = now_epoch();
    node.atime       = node.mtime;
    pit->second.children[name] = node.inode;
    nodes_[np] = node;
    return true;
}

bool DirTree::rename(const std::string& from, const std::string& to) {
    const std::string nf = normalise(from);
    const std::string nt = normalise(to);
    if (!nodes_.count(nf)) return false;
    if (nodes_.count(nt))  return false;
    auto [fp, fname] = split_path(nf);
    auto [tp, tname] = split_path(nt);
    auto fpit = nodes_.find(fp), tpit = nodes_.find(tp);
    if (fpit == nodes_.end() || tpit == nodes_.end()) return false;

    INode node = nodes_[nf];
    nodes_.erase(nf);
    fpit->second.children.erase(fname);
    tpit->second.children[tname] = node.inode;
    nodes_[nt] = node;
    return true;
}

bool DirTree::remove(const std::string& path) {
    const std::string np = normalise(path);
    auto it = nodes_.find(np);
    if (it == nodes_.end()) return false;
    if (it->second.type == NodeType::Directory && !it->second.children.empty()) return false;

    auto [par, name] = split_path(np);
    auto pit = nodes_.find(par);
    if (pit != nodes_.end()) pit->second.children.erase(name);
    nodes_.erase(it);
    return true;
}

std::optional<Stat> DirTree::stat(const std::string& path) const {
    const std::string np = normalise(path);
    auto it = nodes_.find(np);
    if (it == nodes_.end()) return std::nullopt;
    const INode& n = it->second;
    Stat s;
    s.path        = np;
    s.type        = n.type;
    s.size        = n.size;
    s.num_blocks  = n.num_blocks;
    s.replication = n.replication;
    s.mtime       = n.mtime;
    s.atime       = n.atime;
    s.owner       = n.owner;
    s.mode        = n.mode;
    return s;
}

std::optional<std::vector<DirEntry>> DirTree::listdir(const std::string& path) const {
    const std::string np = normalise(path);
    auto it = nodes_.find(np);
    if (it == nodes_.end() || it->second.type != NodeType::Directory) return std::nullopt;
    std::vector<DirEntry> entries;
    for (const auto& [name, ino] : it->second.children) {
        // Find the child node
        for (const auto& [p, n] : nodes_) {
            if (n.inode == ino) {
                entries.push_back({name, n.type, n.inode});
                break;
            }
        }
    }
    return entries;
}

bool DirTree::exists(const std::string& path) const {
    return nodes_.count(normalise(path)) > 0;
}

bool DirTree::set_size(const std::string& path, std::size_t size, std::size_t num_blocks) {
    const std::string np = normalise(path);
    auto it = nodes_.find(np);
    if (it == nodes_.end()) return false;
    it->second.size       = size;
    it->second.num_blocks = num_blocks;
    it->second.mtime      = now_epoch();
    return true;
}

bool DirTree::touch_mtime(const std::string& path) {
    const std::string np = normalise(path);
    auto it = nodes_.find(np);
    if (it == nodes_.end()) return false;
    it->second.mtime = now_epoch();
    return true;
}

} // namespace dfs
} // namespace trekker
