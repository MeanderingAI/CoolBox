#include "metadata_server.h"
#include <algorithm>

namespace trekker {
namespace dfs {

bool MetadataServer::mkdir(const std::string& path, const std::string& owner,
                           std::uint16_t mode) {
    std::lock_guard<std::mutex> lk(mtx_);
    return dir_.mkdir(path, owner, mode);
}

bool MetadataServer::create(const std::string& path, const std::string& owner,
                            std::uint16_t replication) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!dir_.mkfile(path, owner, 0644, replication)) return false;
    block_maps_.emplace(path, BlockMap{});
    return true;
}

bool MetadataServer::rename(const std::string& from, const std::string& to) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!dir_.rename(from, to)) return false;
    auto it = block_maps_.find(from);
    if (it != block_maps_.end()) {
        block_maps_.emplace(to, std::move(it->second));
        block_maps_.erase(it);
    }
    return true;
}

bool MetadataServer::remove(const std::string& path) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!dir_.remove(path)) return false;
    block_maps_.erase(path);
    return true;
}

std::optional<Stat> MetadataServer::stat(const std::string& path) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return dir_.stat(path);
}

std::optional<std::vector<DirEntry>> MetadataServer::listdir(const std::string& path) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return dir_.listdir(path);
}

bool MetadataServer::exists(const std::string& path) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return dir_.exists(path);
}

bool MetadataServer::append_block(const std::string& path, BlockLocation loc) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = block_maps_.find(path);
    if (it == block_maps_.end()) return false;
    it->second.append(loc);
    dir_.set_size(path, it->second.file_size(), it->second.num_blocks());
    return true;
}

std::optional<std::vector<BlockLocation>> MetadataServer::block_locations(
    const std::string& path) const {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = block_maps_.find(path);
    if (it == block_maps_.end()) return std::nullopt;
    return it->second.locations();
}

std::vector<std::string> MetadataServer::replicas_for(const BlockId& id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<std::string> out;
    for (const auto& [path, bmap] : block_maps_)
        for (const auto& loc : bmap.locations())
            if (loc.block_id == id)
                out.push_back(loc.node_id);
    return out;
}

void MetadataServer::register_node(const std::string& node_id, std::size_t cap) {
    std::lock_guard<std::mutex> lk(mtx_);
    nodes_[node_id] = cap;
}

void MetadataServer::deregister_node(const std::string& node_id) {
    std::lock_guard<std::mutex> lk(mtx_);
    nodes_.erase(node_id);
}

std::vector<std::string> MetadataServer::live_nodes() const {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<std::string> out;
    for (const auto& [k, _] : nodes_) out.push_back(k);
    return out;
}

std::string MetadataServer::pick_node_for_write() const {
    std::lock_guard<std::mutex> lk(mtx_);
    if (nodes_.empty()) return "";
    auto it = nodes_.begin();
    std::advance(it, rr_index_ % nodes_.size());
    ++rr_index_;
    return it->first;
}

} // namespace dfs
} // namespace trekker
