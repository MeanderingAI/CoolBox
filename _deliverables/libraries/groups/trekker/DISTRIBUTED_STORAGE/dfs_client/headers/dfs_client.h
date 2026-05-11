#ifndef TREKKER_DFS_CLIENT_H
#define TREKKER_DFS_CLIENT_H

#include "metadata_server.h"
#include "block_store.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace trekker {
namespace dfs {

// ── Open flags ────────────────────────────────────────────────────────────────
enum OpenFlags : int {
    O_DFS_RDONLY = 0x00,
    O_DFS_WRONLY = 0x01,
    O_DFS_RDWR   = 0x02,
    O_DFS_CREATE = 0x04,
    O_DFS_TRUNC  = 0x08,
    O_DFS_APPEND = 0x10,
};

// ── Seek modes ────────────────────────────────────────────────────────────────
enum class SeekMode { Begin, Current, End };

// ── DfsError ──────────────────────────────────────────────────────────────────
enum class DfsError {
    OK,
    NotFound,
    AlreadyExists,
    NotADirectory,
    NotAFile,
    PermissionDenied,
    NoSpace,
    IOError,
    BadFd,
    InvalidArg,
};

// ── FileHandle ────────────────────────────────────────────────────────────────
// Represents an open file in the distributed FS.  Obtained via DfsClient::open().

class FileHandle {
public:
    ~FileHandle() = default;

    // Read up to `count` bytes from the current position.
    // Returns number of bytes actually read; 0 = EOF; -1 = error.
    std::int64_t read(std::uint8_t* buf, std::size_t count);

    // Write `count` bytes at the current position (or end, if append-mode).
    // Returns bytes written; -1 = error.
    std::int64_t write(const std::uint8_t* buf, std::size_t count);

    // Seek within the file.
    std::int64_t seek(std::int64_t offset, SeekMode whence = SeekMode::Begin);

    // Current byte position.
    std::size_t  tell() const { return pos_; }

    // File size at the time of open (cached; call sync() to refresh).
    std::size_t  size() const { return size_; }

    // Is this handle still valid?
    bool         is_open() const { return open_; }

    // Flush write buffer to the block store.
    DfsError     flush();

    // Refresh metadata from the MetadataServer.
    DfsError     sync();

    // Close (also flushes).
    DfsError     close();

    DfsError     last_error() const { return last_err_; }

private:
    friend class DfsClient;

    FileHandle(std::string path, int flags,
               MetadataServer* meta, BlockStore* local_store,
               std::size_t block_size);

    std::string      path_;
    int              flags_;
    MetadataServer*  meta_;
    BlockStore*      store_;
    std::size_t      block_size_;
    std::size_t      pos_        = 0;
    std::size_t      size_       = 0;
    bool             open_       = true;
    bool             dirty_      = false;
    DfsError         last_err_   = DfsError::OK;

    // Current write buffer (one block at a time).
    std::vector<std::uint8_t> write_buf_;
    // Read cache for the last-fetched block.
    std::vector<std::uint8_t> read_cache_;
    std::size_t               cached_block_idx_ = std::size_t(-1);
};

// ── DfsClient ─────────────────────────────────────────────────────────────────
// Primary interface for application code.  Presents a POSIX-like API over
// the distributed FS.
//
// All path strings use POSIX conventions: "/" separator, absolute from root.

class DfsClient {
public:
    struct Config {
        std::size_t block_size       = 4 * 1024 * 1024;  // 4 MiB
        std::string node_id          = "client";
        std::size_t read_cache_blocks = 8;
    };

    // Connect to an in-process MetadataServer + local BlockStore (for testing
    // and single-machine use).  In a real deployment these would be TCP stubs.
    explicit DfsClient(MetadataServer* meta, BlockStore* local_store,
                       Config cfg = {});

    // ── Namespace ─────────────────────────────────────────────────────────
    DfsError mkdir(const std::string& path, bool parents = false,
                   const std::string& owner = "root");
    DfsError rm(const std::string& path, bool recursive = false);
    DfsError rename(const std::string& from, const std::string& to);

    // ── File I/O ──────────────────────────────────────────────────────────
    // Returns nullptr on error; check last_error().
    std::unique_ptr<FileHandle> open(const std::string& path, int flags);

    // Convenience: read entire file into buffer.
    DfsError read_all(const std::string& path, std::vector<std::uint8_t>& out);

    // Convenience: write entire buffer as a new file.
    DfsError write_all(const std::string& path, const std::uint8_t* data,
                       std::size_t len);
    DfsError write_all(const std::string& path, const std::vector<std::uint8_t>& v) {
        return write_all(path, v.data(), v.size());
    }

    // ── Metadata ──────────────────────────────────────────────────────────
    std::optional<Stat>                   stat(const std::string& path) const;
    std::optional<std::vector<DirEntry>>  listdir(const std::string& path) const;
    bool                                  exists(const std::string& path) const;

    DfsError last_error() const { return last_err_; }

private:
    MetadataServer* meta_;
    BlockStore*     store_;
    Config          cfg_;
    DfsError        last_err_ = DfsError::OK;
};

} // namespace dfs
} // namespace trekker

#endif // TREKKER_DFS_CLIENT_H
