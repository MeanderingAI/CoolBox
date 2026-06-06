# database_driver

## Scope
- Add a new app module at `_deliverables/apps/database_driver`.
- Build a `database_driver` executable from `_deliverables/apps/database_driver/src/database_driver_main.cpp`.
- Wire the module into `_deliverables/apps/CMakeLists.txt` so it participates in normal app builds.
- Co-locate related server/client executables (`database_server`, `database_client`) for local or networked SQL usage.

## Build Wiring
- `database_driver` links:
  - `cli_tools`
  - `sql`
- C++ standard: `cxx_std_17`.
- MSVC post-build runtime DLL copy step included to match existing app conventions.

## CLI Surface
- Program: `database_driver`
- Options:
  - `--help`, `-h`
  - `--provider`, `-p` (default `sqlite`)
  - `--db`, `-d` (default `database_driver.sqlite3`)
  - `--query`, `-q` (required unless `--help` is used)

## Behavior
- Connects using `ml::sql::Database::create(provider)`.
- Executes a single SQL statement and exits.
- Prints result columns, rows, affected row count, and last insert id.

## Validation
- Configured and built successfully with:
  - `cmake -S . -B build`
  - `cmake --build build --target database_driver`
- Help command verified:
  - `./build/_deliverables/apps/database_driver/database_driver --help`
