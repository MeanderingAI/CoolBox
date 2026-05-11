# Distributed Setup — host URL fetch bar

**File:** `business_suite/middle_wear/distributed_setup/index.html`  
**Date:** May 9 2026  
**Status:** complete

## What was added

A host URL bar fixed above the page header containing:

- Text input (`id="host-url"`) pre-filled with `http://127.0.0.1:7700/cluster/topology`
- Fetch button (`id="fetch-btn"`) that calls `fetchTopology()`
- Status badge (`id="host-status"`) showing `ok`, `error`, or `timeout`

## Fetch behaviour

`fetchTopology()` is async and uses `AbortSignal.timeout(8000)`. On success it calls `applyTopology(data)`.

## `applyTopology(data)` updates

| UI element | Source field |
|---|---|
| Master status badge + card | `data.master.status` |
| Master PID | `data.master.pid` |
| Jobs per second | `data.master.jobs_per_sec` |
| Queue depth | `data.master.queue_depth` |
| Worker count + topology nodes | `data.workers[]` (rebuilt dynamically) |
| Primitives table rows | `data.primitives[]` (rebuilt dynamically) |
| Jobs processed stat | `data.stats.jobs_processed` |
| Uptime | `data.stats.uptime_seconds` |
| Circuit trips | `data.stats.circuit_trips` |

## Expected JSON shape

```json
{
  "master": { "status": "running", "pid": 1234, "jobs_per_sec": 3.2, "queue_depth": 5 },
  "workers": [{ "id": "worker-1", "status": "running", "jobs_done": 10, "errors": 0 }],
  "primitives": [{ "name": "TaskQueue", "status": "ok", "detail": "depth: 5, processed: 15" }],
  "stats": { "jobs_processed": 15, "uptime_seconds": 42, "circuit_trips": 1 }
}
```
