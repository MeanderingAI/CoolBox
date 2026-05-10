#include "dfs_client.h"
#include <algorithm>
#include <cstring>

namespace trekker {
namespace dfs {

// ── FileHandle ────────────────────────────────────────────────────────────────

FileHandle::FileHandle(std::string path, int flags,
                       MetadataServer* meta, BlockStore* store,
                       std::size_t block_size)
    : path_(std::move(path)), flags_(flags), meta_(meta), store_(store),
      block_size_(block_size) {
    if (auto s = meta_->stat(path_)) size_ = s->size;
}

std::int64_t FileHandle::read(std::uint8_t* buf, std::size_t count) {
    if (!open_ || !(flags_ == O_DFS_RDONLY || flags_ == O_DFS_RDWR)) {
        last_err_ = DfsError::BadFd;
        return -1;
    }
    if (pos_ >= size_) return 0; // EOF
    count = std::min(count, size_ - pos_);

    auto locs = meta_->block_locations(path_);
    if (!locs) { last_err_ = DfsError::IOError; return -1; }

    std::size_t total = 0;
    while (total < count) {
        const std::size_t cur = pos_ + total;
        // Find block covering cur
        BlockId bid{};
        std::size_t blk_off = 0, blk_len = 0;
        for (const auto& loc : *locs) {
            if (cur >= loc.offset && cur < loc.offset + loc.length) {
                bid     = loc.block_id;
                blk_off = cur - loc.offset;
                blk_len = loc.length;
                break;
            }
        }
        auto blk = store_->get(bid);
        if (!blk) { last_err_ = DfsError::IOError; return total > 0 ? total : -1; }
        const std::size_t readable = std::min(blk_len - blk_off, count - total);
        std::memcpy(buf + total, blk->data() + blk_off, readable);
        total += readable;
    }
    pos_ += total;
    return static_cast<std::int64_t>(total);
}

std::int64_t FileHandle::write(const std::uint8_t* buf, std::size_t count) {
    if (!open_ || flags_ == O_DFS_RDONLY) {
        last_err_ = DfsError::BadFd;
        return -1;
    }
    if (flags_ & O_DFS_APPEND) pos_ = size_;

    write_buf_.insert(write_buf_.end(), buf, buf + count);
    dirty_ = true;

    // Flush full blocks eagerly.
    while (write_buf_.size() >= block_size_) {
        std::vector<std::uint8_t> blk(write_buf_.begin(),
                                       write_buf_.begin() + block_size_);
        const BlockId bid = store_->put(blk);
        BlockLocation loc{ "local", bid, pos_, block_size_ };
        meta_->append_block(path_, loc);
        pos_  += block_size_;
        size_  = std::max(size_, pos_);
        write_buf_.erase(write_buf_.begin(), write_buf_.begin() + block_size_);
    }
    return static_cast<std::int64_t>(count);
}

std::int64_t FileHandle::seek(std::int64_t offset, SeekMode whence) {
    if (!open_) { last_err_ = DfsError::BadFd; return -1; }
    std::int64_t base = 0;
    switch (whence) {
        case SeekMode::Begin:   base = 0;                          break;
        case SeekMode::Current: base = static_cast<std::int64_t>(pos_); break;
        case SeekMode::End:     base = static_cast<std::int64_t>(size_); break;
    }
    const std::int64_t np = base + offset;
    if (np < 0) { last_err_ = DfsError::InvalidArg; return -1; }
    pos_ = static_cast<std::size_t>(np);
    return static_cast<std::int64_t>(pos_);
}

DfsError FileHandle::flush() {
    if (!open_) return DfsError::BadFd;
    if (!dirty_ || write_buf_.empty()) return DfsError::OK;
    const BlockId bid = store_->put(write_buf_);
    BlockLocation loc{ "local", bid, pos_, write_buf_.size() };
    meta_->append_block(path_, loc);
    pos_  += write_buf_.size();
    size_  = std::max(size_, pos_);
    write_buf_.clear();
    dirty_ = false;
    return DfsError::OK;
}

DfsError FileHandle::sync() {
    if (!open_) return DfsError::BadFd;
    if (auto s = meta_->stat(path_)) { size_ = s->size; return DfsError::OK; }
    return DfsError::IOError;
}

DfsError FileHandle::close() {
    if (!open_) return DfsError::BadFd;
    const DfsError e = flush();
    open_ = false;
    return e;
}

// ── DfsClient ─────────────────────────────────────────────────────────────────

DfsClient::DfsClient(MetadataServer* meta, BlockStore* store, Config cfg)
    : meta_(meta), store_(store), cfg_(std::move(cfg)) {}

DfsError DfsClient::mkdir(const std::string& path, bool parents,
                          const std::string& owner) {
    if (parents) {
        // Create each component in turn.
        std::string cur;
        for (std::size_t i = 1; i < path.size(); ++i) {
            if (path[i] == '/' || i == path.size() - 1) {
                cur = path.substr(0, i + (path[i] != '/' ? 1 : 0));
                if (!meta_->exists(cur)) meta_->mkdir(cur, owner);
            }
        }
        return DfsError::OK;
    }
    return meta_->mkdir(path, owner) ? DfsError::OK : DfsError::AlreadyExists;
}

DfsError DfsClient::rm(const std::string& path, bool /*recursive*/) {
    return meta_->remove(path) ? DfsError::OK : DfsError::NotFound;
}

DfsError DfsClient::rename(const std::string& from, const std::string& to) {
    return meta_->rename(from, to) ? DfsError::OK : DfsError::NotFound;
}

std::unique_ptr<FileHandle> DfsClient::open(const std::string& path, int flags) {
    if (flags & O_DFS_CREATE) {
        if (!meta_->exists(path)) meta_->create(path);
    }
    if (!meta_->exists(path)) { last_err_ = DfsError::NotFound; return nullptr; }

    auto fh = std::unique_ptr<FileHandle>(
        new FileHandle(path, flags, meta_, store_, cfg_.block_size));
    if (flags & O_DFS_TRUNC) {
        // For simplicity, recreate.
        meta_->remove(path);
        meta_->create(path);
        fh = std::unique_ptr<FileHandle>(
            new FileHandle(path, flags, meta_, store_, cfg_.block_size));
    }
    return fh;
}

DfsError DfsClient::read_all(const std::string& path, std::vector<std::uint8_t>& out) {
    auto fh = open(path, O_DFS_RDONLY);
    if (!fh) return last_err_;
    out.resize(fh->size());
    if (!out.empty()) {
        const std::int64_t n = fh->read(out.data(), out.size());
        if (n < 0) { fh->close(); return DfsError::IOError; }
        out.resize(static_cast<std::size_t>(n));
    }
    return fh->close();
}

DfsError DfsClient::write_all(const std::string& path,
                              const std::uint8_t* data, std::size_t len) {
    auto fh = open(path, O_DFS_CREATE | O_DFS_WRONLY | O_DFS_TRUNC);
    if (!fh) return last_err_;
    fh->write(data, len);
    return fh->close();
}

std::optional<Stat> DfsClient::stat(const std::string& path) const {
    return meta_->stat(path);
}

std::optional<std::vector<DirEntry>> DfsClient::listdir(const std::string& path) const {
    return meta_->listdir(path);
}

bool DfsClient::exists(const std::string& path) const {
    return meta_->exists(path);
}

} // namespace dfs
} // namespace trekker
