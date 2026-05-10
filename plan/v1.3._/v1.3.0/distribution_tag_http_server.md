# distribution_tag_master — embedded HTTP topology server

**Files:** `apps/distribution_tag/master/main.cpp`, `apps/distribution_tag/CMakeLists.txt`  
**Date:** May 9 2026  
**Status:** complete

## Overview

`distribution_tag_master` is a pure C++ CLI app. This change embeds `trekker/IO/http_server` so the master node can serve live cluster state over HTTP while workers are running, enabling the Distributed Setup middleware to poll it.

## Endpoint

| Method | Path | Description |
|---|---|---|
| `GET` | `/cluster/topology` | Live JSON cluster state |
| `GET` | `/health` | 200 OK readiness probe |
| `OPTIONS` | `*` | CORS preflight (`Access-Control-Allow-Origin: *`) |

Default port: **7700** (overridable via `argv[2]`).  
Run signature: `./distribution_tag_master [num_workers [http_port]]`

## New types in main.cpp

### `TopologyState`

Holds const-refs to all live shared global state:

```cpp
struct TopologyState {
    const ServiceRegistry&   registry;
    const TaskQueue<Job>&    job_queue;
    const CircuitBreaker&    breaker;
    const std::atomic<int>&  jobs_done;
    int                      total_jobs;
    int                      num_workers;
    int                      pid;
    std::chrono::steady_clock::time_point started_at;
};
```

### `TopologyServlet`

Inherits `networking::servlets::HttpServletBase`. `build_topology()` assembles the JSON response from `TopologyState`:

- `master` — status, PID, jobs/sec (done ÷ uptime), queue depth
- `workers` — healthy workers from `ServiceRegistry::lookup("workers")` as `"running"`, remainder filled as `"stopped"` up to `num_workers`
- `primitives` — TaskQueue, MessageBus, ServiceRegistry, DistributedLock, CircuitBreaker (circuit state reflected in `status` field)
- `stats` — total processed, uptime seconds, circuit trips

## Lifecycle in main()

1. `CircuitBreaker` and `jobs_attempted` declared
2. `TopologyState tstate{...}` assembled
3. `advanced_logging::Logger topo_log` created (logs to stderr, no file)
4. `HttpServer http_srv(http_port, 2, &topo_log, servlet)` started on background thread
5. Worker threads spawned
6. After all workers and monitor thread join: `http_srv.stop(); http_thread.join()`

## CMakeLists.txt change

```cmake
target_link_libraries(distribution_tag_master PRIVATE distributed http_server Threads::Threads)
```
