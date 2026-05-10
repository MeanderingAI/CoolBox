# DISTRIBUTED_STORAGE Library

## Overview

A new C++ library `distributed_storage` added under `_libraries/groups/trekker/DISTRIBUTED_STORAGE/`.  
Namespace: `trekker::dfs`. CMake target: `distributed_storage`.  
Standard: C++17. Thread-safe throughout via `std::mutex`.

This library provides a distributed file system (DFS) abstraction: content-addressed block storage, a metadata name-node, a POSIX-like client API, replication policy, and a binary chunk-transfer framing layer.

---

## Package Structure

```
DISTRIBUTED_STORAGE/
├── CMakeLists.txt
├── block_store/
│   ├── headers/block_store.h
│   └── source/
│       ├── block_store.cpp
│       └── block_map.cpp
├── metadata_server/
│   ├── headers/metadata_server.h
│   └── source/
│       ├── dir_tree.cpp
│       └── metadata_server.cpp
├── dfs_client/
│   ├── headers/dfs_client.h
│   └── source/
│       ├── dfs_client.cpp
│       └── file_handle.cpp      ← stub (all impl in dfs_client.cpp)
├── replication/
│   ├── headers/replication.h
│   └── source/replication.cpp
└── chunk_transfer/
    ├── headers/chunk_transfer.h
    └── source/chunk_transfer.cpp
```

---

## Package 1 — `block_store`

### Key Types

| Type | Description |
|------|-------------|
| `BlockId` | `std::array<uint8_t, 32>` — 256-bit content hash |
| `BlockMeta` | `{id, size, ref_count, pinned}` |
| `BlockStore` | In-memory content-addressed store |
| `BlockLocation` | `{node_id, block_id, offset, length}` |
| `BlockMap` | Maps a logical file to a list of `BlockLocation` entries |

### `BlockStore` API
- `put(data, len) → BlockId` — content-addresses the block; increments `ref_count` on duplicate
- `get(id) → optional<vector<uint8_t>>`
- `release(id)` — decrements ref count; removes block when it reaches 0 and block is not pinned
- `pin(id)` / `unpin(id)` — prevent eviction
- `meta(id)`, `all_block_ids()`, `block_count()`, `total_bytes()`, `clear()`

### `compute_block_id`
FNV-1a-based 256-bit hash using 4 cross-mixing 64-bit accumulators. Not SHA-256 — a fast content fingerprint.

### Free Functions
- `block_id_to_hex(id) → string`
- `block_id_from_hex(hex) → optional<BlockId>`

---

## Package 2 — `metadata_server`

### Key Types

| Type | Description |
|------|-------------|
| `NodeType` | `File` / `Directory` |
| `Stat` | `{path, type, size, num_blocks, replication, mtime, atime, owner, mode}` |
| `DirEntry` | `{name, type, inode}` |
| `DirTree` | Inode-based filesystem namespace |
| `MetadataServer` | Thread-safe wrapper; holds `DirTree` + `map<string, BlockMap>` |

### `DirTree` internals
- Root node `"/"` created in constructor with `next_inode_ = 1`
- `normalise()` ensures absolute path, removes trailing slash, collapses double slashes
- `listdir()` scans `nodes_` for matching inode numbers

### `MetadataServer` API
- `mkdir(path, parents)`, `create(path)`, `remove(path)`, `rename(src, dst)`
- `stat(path) → optional<Stat>`
- `listdir(path) → vector<DirEntry>`
- `append_block(path, location)` — calls `dir_.set_size()` after appending
- `register_node(node_id, capacity)`, `pick_node_for_write()` — round-robin over registered nodes

---

## Package 3 — `dfs_client`

### Key Types

| Type | Description |
|------|-------------|
| `OpenFlags` | `O_DFS_RDONLY=0`, `WRONLY=1`, `RDWR=2`, `CREATE=4`, `TRUNC=8`, `APPEND=16` |
| `SeekMode` | `Begin`, `Current`, `End` |
| `DfsError` | `OK`, `NotFound`, `AlreadyExists`, `PermissionDenied`, `IOError`, `InvalidArg` |
| `FileHandle` | Returned by `DfsClient::open()` as `unique_ptr<FileHandle>` |
| `DfsClient` | Top-level client API |

### `DfsClient` API
- `mkdir(path, parents=true)`, `rm(path)`, `rename(src, dst)`
- `open(path, flags) → unique_ptr<FileHandle>`
- `read_all(path) → optional<vector<uint8_t>>`
- `write_all(path, data)` — opens with `CREATE|WRONLY|TRUNC`
- `stat(path)`, `listdir(path)`, `exists(path)`, `last_error()`

### `FileHandle` behaviour
- `read(buf, len) → ssize_t` — fetches block by offset range scan
- `write(data, len) → ssize_t` — buffers into `write_buf_`; flushes full blocks eagerly
- `seek(offset, mode)`, `tell()`
- `flush()` — puts remaining `write_buf_` as final block
- `close()` — calls `flush()`

---

## Package 4 — `replication`

### Key Types

| Type | Description |
|------|-------------|
| `PlacementStrategy` | `RoundRobin`, `RackAware`, `Random` |
| `ReplicationConfig` | `{replication_factor=3, strategy, verify_on_read}` |
| `ReplicationPlanner` | `plan(live_nodes, existing_nodes, needed) → vector<string>` |
| `BlockStoreProvider` | `function<BlockStore*(node_id)>` |
| `BlockReplicator` | Coordinates replication across nodes |

### Strategy Details
- **RoundRobin** — `rr_cursor_ % candidates.size()`
- **RackAware** — groups nodes by the first segment before `-` in the node ID
- **Random** — `std::shuffle` with `mt19937`

### `BlockReplicator` API
- `replicate(block_id, source_node, live_nodes)` — gets data from source, puts to each target node
- `rebalance(block_id, live_nodes)` — ensures replication factor is met
- `verify(block_id, live_nodes) → vector<string>` — returns node IDs where the block is missing

---

## Package 5 — `chunk_transfer`

### Key Types

| Type | Description |
|------|-------------|
| `TransferResult` | `{ok, bytes_transferred, error}` |
| `ChunkSender` | Wraps a `WriteFn`; serialises a block as a framed message |
| `ChunkReceiver` | Wraps a `ReadFn`; deserialises a framed message into a `BlockStore` |
| `ChunkPipeline` | In-process `src → dst` pipeline using `BlockStore::get` + `put` |

### Wire Frame Format
```
[ 4B magic (0xDF510000) ][ 32B block_id ][ 8B payload_length ][ payload ]
```

### API
- `ChunkSender::send(id, data) → TransferResult`
- `ChunkReceiver::receive(store) → optional<BlockId>`
- `ChunkPipeline::transfer(ids) → int` — returns count of successful transfers
- `ChunkPipeline::transfer_one(id) → TransferResult`

---

## CMake

```cmake
# Target: distributed_storage
# Sources: all 8 .cpp files across the 5 packages
# Public includes: all 5 headers/ directories

# Test target: distributed_storage_tests
# Links: distributed_storage, tyst_framework_main, Threads::Threads
# CTest: add_test(NAME DistributedStorageTests COMMAND distributed_storage_tests)
```

---

## Tests

| File | Coverage |
|------|----------|
| `block_store/tests/test_block_store.cpp` | put/get roundtrip, dedup, ref_count, pin, total_bytes, BlockMap |
| `metadata_server/tests/test_metadata_server.cpp` | mkdir/listdir, create/stat, rename, remove, append_block, register/pick node |
| `dfs_client/tests/test_dfs_client.cpp` | write_all/read_all, open+write+seek+read, mkdir parents, rm, stat, listdir |
| `replication/tests/test_replication.cpp` | RoundRobin plan, replicate, verify detects missing |
| `chunk_transfer/tests/test_chunk_transfer.cpp` | transfer_one, batch transfer, missing block error, sender/receiver frame roundtrip |

Tests use a manual `EXPECT_TRUE` / `EXPECT_EQ` macro pattern with `int main()` returning `failed > 0 ? 1 : 0`.

---

## Known Design Decisions

- `BlockId` is a fast FNV-1a-based hash, not cryptographic SHA-256.
- `file_handle.cpp` exists as a stub translation unit; all `FileHandle` implementation lives in `dfs_client.cpp`.
- `DirTree` uses a flat `map<inode, INode>` — no trie or B-tree.
- `MetadataServer::pick_node_for_write()` uses simple round-robin with `rr_index_ % nodes_.size()`.
