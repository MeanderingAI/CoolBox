# Distribution Storage App

## Overview

A demo application added under `apps/distribution_storage/` to exercise the `distributed_storage` library interactively.  
Three separate executables are built, each representing a different node role in a distributed file system.

---

## Directory Structure

```
apps/distribution_storage/
├── CMakeLists.txt
├── shared/
│   └── dfs_common.h          ← shared CLI utilities
├── storage_node/
│   └── main.cpp              ← data node REPL
├── metadata_node/
│   └── main.cpp              ← name node REPL
└── dfs_shell/
    └── main.cpp              ← all-in-one DFS REPL
```

---

## Shared Utilities — `dfs_common.h`

Namespace: `dfs_app`.

| Function | Description |
|----------|-------------|
| `parse_host_port(s)` | Splits `"host:port"` strings |
| `format_size(bytes)` | Returns human-readable size string (B / KiB / MiB / GiB) |
| `print_stat(stat)` | Pretty-prints a `MetadataServer::Stat` |
| `print_listing(entries)` | Pretty-prints a `vector<DirEntry>` |

---

## Executable 1 — `distribution_storage_node`

**Role:** Data node — interactive block store.

**Arguments:**
- `--node-id <id>` — identifier for this node
- `--capacity <bytes>` — optional storage cap

**REPL Commands:**

| Command | Description |
|---------|-------------|
| `put <data>` | Store raw string data as a block; prints hex block ID |
| `get <hex_id>` | Retrieve and print block contents |
| `stats` | Print block count, total bytes |
| `exit` | Quit |

---

## Executable 2 — `distribution_storage_meta`

**Role:** Name node — interactive filesystem namespace.

**Arguments:**
- `--nodes n1,n2,...` — comma-separated list of data node IDs to register

**REPL Commands:**

| Command | Description |
|---------|-------------|
| `ls [path]` | List directory contents |
| `stat <path>` | Print file/directory metadata |
| `mkdir <path>` | Create directory (with parents) |
| `rm <path>` | Remove a file or directory |
| `nodes` | List registered data nodes |
| `pick` | Show which node round-robin would select next |
| `exit` | Quit |

---

## Executable 3 — `dfs_shell`

**Role:** All-in-one interactive DFS demo. Runs `MetadataServer`, `BlockStore`, and `DfsClient` in-process — no networking required.

**REPL Commands:**

| Command | Description |
|---------|-------------|
| `ls [path]` | List directory |
| `stat <path>` | File/directory metadata |
| `mkdir <path>` | Create directory (parents created automatically) |
| `put <local> <dfs>` | Read local file via `std::ifstream`, write to DFS path |
| `get <dfs> <local>` | Read from DFS, write to local file via `std::ofstream` |
| `cat <path>` | Print DFS file contents to stdout |
| `rm <path>` | Remove file |
| `help` | Print command reference |
| `exit` | Quit |

---

## CMake

```cmake
# DS_HEADERS: list of all 5 package header directories
# Targets: distribution_storage_node, distribution_storage_meta, dfs_shell
# All link: distributed_storage, Threads::Threads
# All include: shared/ + ${DS_HEADERS}
# CXX_STANDARD: 17
```

`apps/CMakeLists.txt` includes this subdirectory with an `if(EXISTS ...)` guard, consistent with the existing apps pattern.

---

## Design Notes

- All three executables are in-process only — there is no real socket networking. The app demonstrates the library API, not a production deployment.
- `dfs_shell` is the primary entry point for interactive exploration; `storage_node` and `metadata_node` exist to show how the roles would be separated in a networked deployment.
