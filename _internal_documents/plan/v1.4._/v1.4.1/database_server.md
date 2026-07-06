# database_server

## Scope
- Add a TCP SQL server executable under `_deliverables/apps/database_driver`.
- Expose a port that `database_client` can connect to for remote SQL execution.

## Build Wiring
- Target: `database_server`
- Source: `_deliverables/apps/database_driver/src/database_server_main.cpp`
- Links:
  - `cli_tools`
  - `sql`
- C++ standard: `cxx_std_17`

## CLI Surface
- Program: `database_server`
- Options:
  - `--help`, `-h`
  - `--provider`, `-p` (default `sqlite`)
  - `--db`, `-d` (default `database_driver.sqlite3`)
  - `--host` (default `127.0.0.1`)
  - `--port` (default `55432`)

## Behavior
- Opens a SQLite-backed database through existing `ml::sql::Database` APIs.
- Binds/listens on the configured host/port.
- Executes incoming SQL lines and returns execution output.
- Supports transaction SQL statements directly (`BEGIN`, `COMMIT`, `ROLLBACK`) because they pass through to SQLite execution.

## Notes
- Current implementation targets POSIX sockets (macOS/Linux).
