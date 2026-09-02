# database_client

## Scope
- Add a companion CLI client executable in `_deliverables/apps/database_driver`.
- Build `database_client` from `_deliverables/apps/database_driver/src/database_client_main.cpp`.
- Reuse OS_GENERICS `cli_tools` for argument parsing.

## Build Wiring
- `database_client` links:
  - `cli_tools`
  - `sql`
- C++ standard: `cxx_std_17`.
- MSVC post-build runtime DLL copy step included.

## CLI Surface
- Program: `database_client`
- Options:
  - `--help`, `-h`
  - `--provider`, `-p` (default `sqlite`)
  - `--db`, `-d` (default `database_driver.sqlite3`)
  - `--host` (enables remote TCP mode)
  - `--port` (default `55432`)

## Behavior
- Starts an interactive prompt:
  - Prompt: `db> `
  - Exit commands: `.exit`, `exit`, `quit`
- Local mode (default): opens a DB connection using `ml::sql::Database` and executes directly.
- Remote mode (`--host`): connects to `database_server` over TCP and executes SQL remotely.
- Executes each entered SQL line and prints result rows/metadata.

## Validation
- Built successfully with:
  - `cmake --build build --target database_client`
- Help command verified:
  - `./build/_deliverables/apps/database_driver/database_client --help`
